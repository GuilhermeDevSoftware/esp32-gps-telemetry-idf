import argparse
import json
import sys
from io import StringIO
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


REQUIRED_COLUMNS = [
    "utc",
    "latitude",
    "longitude",
    "speed_kmh",
    "max_speed_kmh",
    "avg_speed_kmh",
    "total_distance_m",
    "status",
    "stopped_time_s",
    "moving_time_s",
    "satellites",
    "hdop",
]

NUMERIC_COLUMNS = [
    "utc",
    "latitude",
    "longitude",
    "speed_kmh",
    "max_speed_kmh",
    "avg_speed_kmh",
    "total_distance_m",
    "stopped_time_s",
    "moving_time_s",
    "satellites",
    "hdop",
]


def read_csv_clean(csv_path: Path) -> pd.DataFrame:
    """
    Lê o CSV exportado do ESP32.

    Aceita tanto CSV puro quanto texto copiado do monitor contendo:
    ---CSV_BEGIN_SESSION_009---
    ...
    ---CSV_END_SESSION_009---
    """

    if not csv_path.exists():
        raise FileNotFoundError(f"Arquivo não encontrado: {csv_path}")

    raw_text = csv_path.read_text(encoding="utf-8", errors="ignore")

    clean_lines = []

    for line in raw_text.splitlines():
        line = line.strip()

        if not line:
            continue

        if line.startswith("---CSV_BEGIN"):
            continue

        if line.startswith("---CSV_END"):
            continue

        clean_lines.append(line)

    if not clean_lines:
        raise ValueError("O arquivo está vazio ou não contém dados CSV válidos.")

    clean_text = "\n".join(clean_lines)

    return pd.read_csv(StringIO(clean_text))


def validate_columns(df: pd.DataFrame) -> None:
    missing_columns = [col for col in REQUIRED_COLUMNS if col not in df.columns]

    if missing_columns:
        raise ValueError(
            "CSV inválido. Colunas ausentes: "
            + ", ".join(missing_columns)
        )


def normalize_dataframe(df: pd.DataFrame) -> pd.DataFrame:
    df = df.copy()

    for col in NUMERIC_COLUMNS:
        df[col] = pd.to_numeric(df[col], errors="coerce")

    df["status"] = df["status"].astype(str).str.upper().str.strip()

    df = df.dropna(subset=["latitude", "longitude", "speed_kmh"])

    if df.empty:
        raise ValueError("Nenhuma linha válida encontrada após normalização.")

    return df


def get_last_valid_value(df: pd.DataFrame, column: str, default: float = 0.0) -> float:
    valid_values = df[column].dropna()

    if valid_values.empty:
        return default

    return float(valid_values.iloc[-1])


def classify_gps_quality(avg_satellites: float, avg_hdop: float) -> str:
    if avg_satellites >= 8 and avg_hdop <= 1.5:
        return "BOA"

    if avg_satellites >= 4 and avg_hdop <= 3.0:
        return "REGULAR"

    return "RUIM"


def build_summary(df: pd.DataFrame, csv_path: Path) -> dict:
    moving_df = df[df["status"] == "MOVING"]
    stopped_df = df[df["status"] == "STOPPED"]

    total_points = len(df)

    distance_total_m = get_last_valid_value(df, "total_distance_m")
    firmware_max_speed_kmh = get_last_valid_value(df, "max_speed_kmh")
    calculated_max_speed_kmh = float(df["speed_kmh"].max())

    firmware_avg_speed_kmh = get_last_valid_value(df, "avg_speed_kmh")
    calculated_avg_speed_kmh = float(df["speed_kmh"].mean())

    if not moving_df.empty:
        avg_moving_speed_kmh = float(moving_df["speed_kmh"].mean())
    else:
        avg_moving_speed_kmh = 0.0

    stopped_time_s = get_last_valid_value(df, "stopped_time_s")
    moving_time_s = get_last_valid_value(df, "moving_time_s")
    total_time_s = stopped_time_s + moving_time_s

    avg_satellites = float(df["satellites"].mean())
    min_satellites = int(df["satellites"].min())
    max_satellites = int(df["satellites"].max())

    avg_hdop = float(df["hdop"].mean())
    min_hdop = float(df["hdop"].min())
    max_hdop = float(df["hdop"].max())

    gps_quality = classify_gps_quality(avg_satellites, avg_hdop)

    status_counts = df["status"].value_counts().to_dict()

    return {
        "file": csv_path.name,
        "total_points": total_points,
        "distance_total_m": round(distance_total_m, 2),
        "distance_total_km": round(distance_total_m / 1000.0, 3),
        "firmware_max_speed_kmh": round(firmware_max_speed_kmh, 2),
        "calculated_max_speed_kmh": round(calculated_max_speed_kmh, 2),
        "firmware_avg_speed_kmh": round(firmware_avg_speed_kmh, 2),
        "calculated_avg_speed_kmh": round(calculated_avg_speed_kmh, 2),
        "avg_moving_speed_kmh": round(avg_moving_speed_kmh, 2),
        "stopped_time_s": int(stopped_time_s),
        "moving_time_s": int(moving_time_s),
        "total_time_s": int(total_time_s),
        "moving_points": int(len(moving_df)),
        "stopped_points": int(len(stopped_df)),
        "status_counts": status_counts,
        "avg_satellites": round(avg_satellites, 2),
        "min_satellites": min_satellites,
        "max_satellites": max_satellites,
        "avg_hdop": round(avg_hdop, 2),
        "min_hdop": round(min_hdop, 2),
        "max_hdop": round(max_hdop, 2),
        "gps_quality": gps_quality,
    }


def format_seconds(seconds: int) -> str:
    minutes = seconds // 60
    remaining_seconds = seconds % 60

    if minutes == 0:
        return f"{remaining_seconds} s"

    return f"{minutes} min {remaining_seconds} s"


def print_summary(summary: dict) -> None:
    print()
    print("====================================")
    print(" RELATÓRIO DA SESSÃO GPS")
    print("====================================")
    print(f"Arquivo: {summary['file']}")
    print(f"Pontos coletados: {summary['total_points']}")
    print(f"Distância total: {summary['distance_total_m']} m")
    print(f"Distância total: {summary['distance_total_km']} km")
    print()
    print(f"Velocidade máxima registrada no firmware: {summary['firmware_max_speed_kmh']} km/h")
    print(f"Velocidade máxima recalculada no Python: {summary['calculated_max_speed_kmh']} km/h")
    print(f"Velocidade média registrada no firmware: {summary['firmware_avg_speed_kmh']} km/h")
    print(f"Velocidade média recalculada no Python: {summary['calculated_avg_speed_kmh']} km/h")
    print(f"Velocidade média em movimento: {summary['avg_moving_speed_kmh']} km/h")
    print()
    print(f"Tempo parado: {format_seconds(summary['stopped_time_s'])}")
    print(f"Tempo em movimento: {format_seconds(summary['moving_time_s'])}")
    print(f"Tempo total estimado: {format_seconds(summary['total_time_s'])}")
    print()
    print(f"Pontos MOVING: {summary['moving_points']}")
    print(f"Pontos STOPPED: {summary['stopped_points']}")
    print()
    print(f"Satélites médios: {summary['avg_satellites']}")
    print(f"Satélites mínimo/máximo: {summary['min_satellites']} / {summary['max_satellites']}")
    print(f"HDOP médio: {summary['avg_hdop']}")
    print(f"HDOP mínimo/máximo: {summary['min_hdop']} / {summary['max_hdop']}")
    print(f"Qualidade GPS: {summary['gps_quality']}")
    print("====================================")
    print()


def save_summary_files(summary: dict, report_dir: Path, session_name: str) -> None:
    report_dir.mkdir(parents=True, exist_ok=True)

    json_path = report_dir / f"{session_name}_summary.json"
    txt_path = report_dir / f"{session_name}_summary.txt"

    json_path.write_text(
        json.dumps(summary, indent=4, ensure_ascii=False),
        encoding="utf-8",
    )

    lines = [
        "RELATÓRIO DA SESSÃO GPS",
        "========================",
        f"Arquivo: {summary['file']}",
        f"Pontos coletados: {summary['total_points']}",
        f"Distância total: {summary['distance_total_m']} m",
        f"Distância total: {summary['distance_total_km']} km",
        "",
        f"Velocidade máxima registrada no firmware: {summary['firmware_max_speed_kmh']} km/h",
        f"Velocidade máxima recalculada no Python: {summary['calculated_max_speed_kmh']} km/h",
        f"Velocidade média registrada no firmware: {summary['firmware_avg_speed_kmh']} km/h",
        f"Velocidade média recalculada no Python: {summary['calculated_avg_speed_kmh']} km/h",
        f"Velocidade média em movimento: {summary['avg_moving_speed_kmh']} km/h",
        "",
        f"Tempo parado: {format_seconds(summary['stopped_time_s'])}",
        f"Tempo em movimento: {format_seconds(summary['moving_time_s'])}",
        f"Tempo total estimado: {format_seconds(summary['total_time_s'])}",
        "",
        f"Pontos MOVING: {summary['moving_points']}",
        f"Pontos STOPPED: {summary['stopped_points']}",
        "",
        f"Satélites médios: {summary['avg_satellites']}",
        f"Satélites mínimo/máximo: {summary['min_satellites']} / {summary['max_satellites']}",
        f"HDOP médio: {summary['avg_hdop']}",
        f"HDOP mínimo/máximo: {summary['min_hdop']} / {summary['max_hdop']}",
        f"Qualidade GPS: {summary['gps_quality']}",
    ]

    txt_path.write_text("\n".join(lines), encoding="utf-8")

    print(f"Relatório JSON salvo em: {json_path}")
    print(f"Relatório TXT salvo em: {txt_path}")


def generate_plots(df: pd.DataFrame, report_dir: Path, session_name: str) -> None:
    report_dir.mkdir(parents=True, exist_ok=True)

    sample_index = range(len(df))

    speed_plot_path = report_dir / f"{session_name}_speed.png"
    distance_plot_path = report_dir / f"{session_name}_distance.png"
    gps_quality_plot_path = report_dir / f"{session_name}_gps_quality.png"

    plt.figure(figsize=(10, 4))
    plt.plot(sample_index, df["speed_kmh"], marker="o")
    plt.axhline(5, linestyle="--", label="Limite MOVING/STOPPED: 5 km/h")
    plt.title("Velocidade ao longo da sessão")
    plt.xlabel("Amostra")
    plt.ylabel("Velocidade (km/h)")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(speed_plot_path)
    plt.close()

    plt.figure(figsize=(10, 4))
    plt.plot(sample_index, df["total_distance_m"], marker="o")
    plt.title("Distância acumulada")
    plt.xlabel("Amostra")
    plt.ylabel("Distância total (m)")
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(distance_plot_path)
    plt.close()

    plt.figure(figsize=(10, 4))
    plt.plot(sample_index, df["satellites"], marker="o", label="Satélites")
    plt.plot(sample_index, df["hdop"], marker="o", label="HDOP")
    plt.title("Qualidade do sinal GPS")
    plt.xlabel("Amostra")
    plt.ylabel("Valor")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(gps_quality_plot_path)
    plt.close()

    print(f"Gráfico de velocidade salvo em: {speed_plot_path}")
    print(f"Gráfico de distância salvo em: {distance_plot_path}")
    print(f"Gráfico de qualidade GPS salvo em: {gps_quality_plot_path}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Analisador de sessões GPS exportadas pelo ESP32."
    )

    parser.add_argument(
        "csv_file",
        help="Caminho para o arquivo CSV da sessão GPS.",
    )

    parser.add_argument(
        "--out",
        default=None,
        help="Pasta de saída para relatórios e gráficos.",
    )

    args = parser.parse_args()

    csv_path = Path(args.csv_file)

    script_dir = Path(__file__).resolve().parent

    if args.out:
        report_dir = Path(args.out)
    else:
        report_dir = script_dir / "reports"

    try:
        df = read_csv_clean(csv_path)
        validate_columns(df)
        df = normalize_dataframe(df)

        session_name = csv_path.stem

        summary = build_summary(df, csv_path)

        print_summary(summary)
        save_summary_files(summary, report_dir, session_name)
        generate_plots(df, report_dir, session_name)

        return 0

    except Exception as error:
        print()
        print("Erro ao analisar a sessão GPS.")
        print(f"Detalhe: {error}")
        print()
        return 1


if __name__ == "__main__":
    sys.exit(main())
