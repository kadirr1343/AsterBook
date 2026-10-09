#!/usr/bin/env python3
"""Render measured benchmark rows; no dependencies or remote assets."""
import argparse, csv, html, json, math, pathlib, statistics

def render(bench, output):
    rows=list(csv.DictReader(bench.open()))
    if not rows:
        raise ValueError('no benchmark rows')
    metrics=['throughput_ops_s','p50_ns','p95_ns','p99_ns','max_ns','timer_ns']
    for row in rows:
        for metric in metrics:
            value=float(row[metric])
            if not math.isfinite(value) or value<=0:
                raise ValueError('invalid measured value')
        if int(row['commands'])<=0:
            raise ValueError('no samples')
    meta_path=bench.with_suffix('.meta.json')
    meta=json.loads(meta_path.read_text()) if meta_path.exists() else {'metadata':'not supplied: platform/flags unknown'}
    groups={}
    for row in rows:
        groups.setdefault(row['engine'],[]).append(row)
    bars=[]
    maximum=max(statistics.median(float(r['throughput_ops_s']) for r in group) for group in groups.values())
    for index,(engine,group) in enumerate(groups.items()):
        value=statistics.median(float(r['throughput_ops_s']) for r in group)
        y=25+index*45
        bars.append(f'<text x="0" y="{y}">{html.escape(engine)}</text><rect x="130" y="{y-15}" width="{value/maximum*400:.1f}" height="20" fill="#2563eb"/><text x="540" y="{y}">{value:,.0f} ops/s</text>')
    keys=list(rows[0])
    table='<table><thead><tr>'+''.join('<th>'+html.escape(k)+'</th>' for k in keys)+'</tr></thead><tbody>'
    for row in rows:
        table+='<tr>'+''.join('<td>'+html.escape(row[k])+'</td>' for k in keys)+'</tr>'
    table+='</tbody></table>'
    document="""<!doctype html><html lang="en"><meta charset="utf-8"><title>AsterBook measurements</title><style>body{font:16px system-ui;margin:2rem;color:#172033}table{border-collapse:collapse;font-size:13px}td,th{border:1px solid #ccc;padding:6px}pre{white-space:pre-wrap}svg{max-width:100%}</style><h1>AsterBook measured engine calls</h1><p>Throughput: median of repetitions, ops/s. Latency: nanoseconds, separate pass, nearest-rank quantiles per repetition. Small samples and OS jitter limit tail interpretation. One fresh-state warmup, five measured repetitions; event allocation and result retention included; parsing and output excluded. No CPU pinning or profiling claim.</p>"""
    document+=f'<svg role="img" aria-label="Median throughput in operations per second" viewBox="0 0 760 {len(groups)*45+10}">'+''.join(bars)+'</svg>'
    document+='<h2>Raw repetitions</h2>'+table+'<h2>Platform and corpus metadata</h2><pre>'+html.escape(json.dumps(meta,indent=2))+'</pre></html>'
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(document)

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--bench',type=pathlib.Path,required=True);parser.add_argument('--output',type=pathlib.Path,required=True);args=parser.parse_args()
    try: render(args.bench,args.output)
    except (ValueError,KeyError,OSError) as e: parser.exit(1,str(e)+'\n')
