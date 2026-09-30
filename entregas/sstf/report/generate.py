#!/usr/bin/env python3
"""Gera métricas e relatório PDF a partir dos logs reais ou da tabela do TP2."""

import argparse
import csv
import re
from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.pagesizes import A4, landscape
from reportlab.pdfbase.pdfmetrics import stringWidth
from reportlab.pdfgen import canvas

ROOT = Path(__file__).resolve().parent
LOG_PATTERN = re.compile(r"\[SSTF\]\s+(add|dsp)\s+([RW])\s+(\d+)")
NAVY = colors.HexColor("#17324D")
BLUE = colors.HexColor("#2667A8")
ORANGE = colors.HexColor("#DC7135")
GRAY = colors.HexColor("#5C6975")
LIGHT = colors.HexColor("#EAF0F5")


def movement(sectors):
    return sum(abs(current - previous)
               for previous, current in zip(sectors, sectors[1:]))


def reference_data():
    with (ROOT / "referencia_tp2.csv").open(newline="") as stream:
        arrivals = [int(row["setor"]) for row in csv.DictReader(stream)]
    pending = list(arrivals)
    dispatch = []
    head = 0
    while pending:
        index = min(range(len(pending)),
                    key=lambda i: abs(pending[i] - head))
        head = pending.pop(index)
        dispatch.append(head)
    assert len(arrivals) == 50
    assert movement(arrivals) == 37_074_272
    return arrivals, dispatch


def log_data(path):
    arrivals, dispatch = [], []
    for line in path.read_text(errors="replace").splitlines():
        match = LOG_PATTERN.search(line)
        if not match or match.group(2) != "R":
            continue
        target = arrivals if match.group(1) == "add" else dispatch
        target.append(int(match.group(3)))
    if len(arrivals) < 2 or len(dispatch) < 2:
        raise ValueError("O log precisa conter pelo menos duas chegadas e dois despachos de leitura")
    return arrivals, dispatch


def write_csv(path, arrivals, dispatch):
    with path.open("w", newline="") as stream:
        writer = csv.writer(stream, lineterminator="\n")
        writer.writerow(["ordem", "setor_chegada", "setor_despacho"])
        for index in range(max(len(arrivals), len(dispatch))):
            writer.writerow([
                index + 1,
                arrivals[index] if index < len(arrivals) else "",
                dispatch[index] if index < len(dispatch) else "",
            ])


def line(pdf, x, y, value, font="Helvetica", size=10, color=NAVY):
    pdf.setFillColor(color)
    pdf.setFont(font, size)
    pdf.drawString(x, y, value)


def wrapped(pdf, x, y, text, width, size=10, leading=15):
    words = text.split()
    current = ""
    for word in words:
        candidate = f"{current} {word}".strip()
        if stringWidth(candidate, "Helvetica", size) > width and current:
            line(pdf, x, y, current, size=size, color=GRAY)
            y -= leading
            current = word
        else:
            current = candidate
    if current:
        line(pdf, x, y, current, size=size, color=GRAY)
        y -= leading
    return y


def plot_sequence(pdf, arrivals, dispatch, x, y, width, height):
    maximum = max(arrivals + dispatch) or 1
    longest = max(len(arrivals), len(dispatch)) - 1
    pdf.setStrokeColor(LIGHT)
    for tick in range(5):
        py = y + height * tick / 4
        pdf.line(x, py, x + width, py)
        line(pdf, x - 55, py - 3, f"{int(maximum * tick / 4):,}",
             size=7, color=GRAY)
    for sequence, color in ((arrivals, BLUE), (dispatch, ORANGE)):
        pdf.setStrokeColor(color)
        pdf.setLineWidth(1.6)
        path = pdf.beginPath()
        for index, sector in enumerate(sequence):
            px = x + width * index / max(longest, 1)
            py = y + height * sector / maximum
            if index == 0:
                path.moveTo(px, py)
            else:
                path.lineTo(px, py)
        pdf.drawPath(path)
    line(pdf, x, y - 19, "Ordem das requisições", size=9, color=GRAY)
    line(pdf, x + 215, y - 19, "Azul: chegada", size=9, color=BLUE)
    line(pdf, x + 325, y - 19, "Laranja: despacho", size=9, color=ORANGE)


def plot_bars(pdf, arrival_distance, dispatch_distance):
    line(pdf, 42, 218, "Percurso total entre setores consecutivos",
         font="Helvetica-Bold", size=13)
    longest = max(arrival_distance, dispatch_distance, 1)
    for y, label, value, color in (
        (166, "Chegada", arrival_distance, BLUE),
        (113, "Despacho", dispatch_distance, ORANGE),
    ):
        line(pdf, 42, y + 9, label, font="Helvetica-Bold", size=10)
        pdf.setFillColor(LIGHT)
        pdf.rect(158, y, 506, 27, fill=1, stroke=0)
        pdf.setFillColor(color)
        pdf.rect(158, y, 506 * value / longest, 27, fill=1, stroke=0)
        line(pdf, 675, y + 9, f"{value:,} setores", size=9)


def make_pdf(path, arrivals, dispatch, reference):
    width, height = landscape(A4)
    pdf = canvas.Canvas(str(path), pagesize=(width, height), pageCompression=1)
    pdf.setTitle("SSTF - relatório de escalonamento de disco")
    pdf.setAuthor("Grupo J - Laboratório de Sistemas Operacionais")
    pdf.setSubject("Comparação da ordem de chegada e despacho de requisições")
    title = "SSTF: escalonamento de disco"
    subtitle = ("REFERÊNCIA ANALÍTICA - sem medição no Codespace"
                if reference else
                "EXECUÇÃO REAL - logs do kernel 4.13.9 no QEMU")
    arrival_distance = movement(arrivals)
    dispatch_distance = movement(dispatch)
    change = 100 * (arrival_distance - dispatch_distance) / arrival_distance if arrival_distance else 0

    pdf.setFillColor(NAVY)
    pdf.rect(0, height - 92, width, 92, fill=1, stroke=0)
    line(pdf, 42, height - 42, title, font="Helvetica-Bold", size=22,
         color=colors.white)
    line(pdf, 42, height - 67, subtitle, size=11, color=colors.white)
    line(pdf, 42, height - 122,
         f"Chegadas: {len(arrivals)}   |   Despachos: {len(dispatch)}",
         font="Helvetica-Bold", size=12)
    line(pdf, 42, height - 148,
         f"Percurso pela ordem de chegada: {arrival_distance:,} setores",
         size=11, color=BLUE)
    line(pdf, 42, height - 169,
         f"Percurso pela ordem de despacho: {dispatch_distance:,} setores",
         size=11, color=ORANGE)
    line(pdf, 42, height - 190,
         f"Redução nesta métrica: {change:.1f}%",
         font="Helvetica-Bold", size=11)
    line(pdf, 97, height - 229,
         "Setor inicial por requisição (ordem de chegada e atendimento)",
         font="Helvetica-Bold", size=12)
    plot_sequence(pdf, arrivals, dispatch, 97, 79, width - 160, height - 334)
    pdf.showPage()

    line(pdf, 42, height - 48, "Método e interpretação",
         font="Helvetica-Bold", size=20)
    y = height - 86
    paragraphs = [
        "Métrica: soma das distâncias absolutas entre setores iniciais consecutivos. "
        "O primeiro deslocamento, da posição inicial da cabeça ao primeiro setor, "
        "não entra na soma, como na tabela de referência do TP2.",
        "Política implementada: a cada despacho o módulo escolhe, entre as "
        "requisições já pendentes, a de menor distância ao fim da última "
        "requisição despachada. Empates preservam a ordem de chegada.",
        "O gerador cria filhos concorrentes, usa leituras O_DIRECT de 4096 bytes "
        "alinhadas e desabilita a mesclagem e o read-ahead do disco de testes. "
        "Assim, a fila tem mais chances de conter várias requisições pendentes.",
    ]
    for paragraph in paragraphs:
        y = wrapped(pdf, 42, y, paragraph, width - 84, size=11, leading=17) - 19
    if reference:
        y = wrapped(pdf, 42, y,
            "Este PDF usa os 50 setores do enunciado. A curva laranja é o limite "
            "analítico obtido quando as 50 requisições já estão disponíveis desde "
            "o início; não é resultado medido do módulo. O exemplo online do PDF "
            "da disciplina informa 7.571.176 setores, pois as chegadas ocorrem "
            "durante a execução. Execute run_sstf no QEMU e lab-collect.sh no "
            "Codespace para produzir o relatório experimental SSTF.pdf.",
            width - 84, size=11, leading=17)
    else:
        y = wrapped(pdf, 42, y,
            "Os dados vieram de dmesg no kernel convidado. Como o QEMU emula "
            "o disco e o host também agenda I/O, tempos de relógio não são "
            "comparáveis. A sequência de setores é a evidência principal. "
            "Diferenças entre contagens de chegada e despacho podem indicar "
            "mesclagem, perda de logs ou eventos extras e devem ser investigadas.",
            width - 84, size=11, leading=17)
    y -= 24
    line(pdf, 42, y, "Limitações", font="Helvetica-Bold", size=13)
    y -= 23
    wrapped(pdf, 42, y,
        "O escalonador SSTF pode causar espera indefinida para setores distantes "
        "sob carga contínua. A métrica usa setores iniciais; o módulo considera "
        "o fim da requisição anterior para escolher a próxima. O disco virtual "
        "não permite inferir ganhos de tempo em um disco físico.",
        width - 84, size=11, leading=17)
    plot_bars(pdf, arrival_distance, dispatch_distance)
    line(pdf, 42, 37, "Grupo J | TP2 | Linux 4.13.9 | dados e script acompanham o relatório",
         size=9, color=GRAY)
    pdf.save()


def main():
    parser = argparse.ArgumentParser()
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--reference", action="store_true")
    source.add_argument("--log", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    arrivals, dispatch = (reference_data() if args.reference
                          else log_data(args.log))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    csv_path = args.output.with_name(args.output.stem + "-ordens.csv")
    write_csv(csv_path, arrivals, dispatch)
    make_pdf(args.output, arrivals, dispatch, args.reference)
    print(f"PDF: {args.output}")
    print(f"Dados: {csv_path}")
    print(f"Percurso: {movement(arrivals):,} -> {movement(dispatch):,} setores")


if __name__ == "__main__":
    main()
