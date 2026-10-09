#!/usr/bin/env python3
"""Record corpus, exact binary/source identity, environment and raw measurements."""
import csv, argparse, hashlib, json, pathlib, platform, subprocess
from datetime import datetime
from zoneinfo import ZoneInfo
ROOT=pathlib.Path(__file__).resolve().parents[1]
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def command(args): return subprocess.check_output(args,text=True,cwd=ROOT).strip()
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--binary',type=pathlib.Path,default=ROOT/'build/release/asterbook');parser.add_argument('--output',type=pathlib.Path,default=ROOT/'benchmarks/results');args=parser.parse_args();binary=args.binary.resolve();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    source=hashlib.sha256()
    paths=[ROOT/'CMakeLists.txt',ROOT/'CMakePresets.json']
    for folder in ['include','src','apps','tests','tools']:
        paths.extend(f for f in (ROOT/folder).rglob('*') if f.is_file() and '__pycache__' not in str(f))
    paths.append(ROOT/'benchmarks/harness.cpp')
    for f in sorted(paths):source.update(str(f.relative_to(ROOT)).encode()+b'\0'+f.read_bytes())
    cpu=next((line.split(':',1)[1].strip() for line in pathlib.Path('/proc/cpuinfo').read_text().splitlines() if line.startswith('model name')),'unknown')
    cache=binary.parent/'CMakeCache.txt'
    compiler=next((line.split('=',1)[1] for line in cache.read_text().splitlines() if line.startswith('CMAKE_CXX_COMPILER:FILEPATH=')),'g++') if cache.exists() else 'g++'
    version=command([compiler,'--version']).splitlines()[0]
    macros=command([compiler,'-dM','-E','-x','c++','-include','bits/c++config.h','/dev/null'])
    stdlib='; '.join(line for line in macros.splitlines() if line.startswith('#define __GLIBCXX__ ') or line.startswith('#define _GLIBCXX_RELEASE '))
    rev=subprocess.run(['git','rev-parse','HEAD'],cwd=ROOT,capture_output=True,text=True)
    revision=rev.stdout.strip() if rev.returncode==0 else 'no Git commit at measurement'
    if rev.returncode==0 and command(['git','status','--porcelain']):revision+=' (dirty working tree; source digest is authoritative)'
    ninja=binary.parent/'build.ninja'
    flags=sorted(set(line.strip() for line in ninja.read_text().splitlines() if line.strip().startswith('FLAGS ='))) if ninja.exists() else ['unavailable']
    for scenario in ['add-only','cancel-heavy','mixed','sweep-heavy']:
        for count,levels in [(1000,8),(6000,128)]:
            name=f'{scenario}-{count}-{levels}';corpus=out/(name+'.commands.csv');bench=out/(name+'.csv')
            subprocess.run([str(binary),'generate','--seed','42','--count',str(count),'--scenario',scenario,'--levels',str(levels),'--output',str(corpus)],check=True)
            subprocess.run([str(binary),'verify','--input',str(corpus)],check=True)
            events=out/(name+'.events.csv');snapshot=out/(name+'.snapshot.csv')
            subprocess.run([str(binary),'replay','--input',str(corpus),'--engine','price-level','--events',str(events),'--snapshot',str(snapshot)],check=True)
            active={};peak_active=0;peak_levels=0
            for event in csv.DictReader(events.open()):
                key=(event['symbol'],event['id']);kind=int(event['kind']);qty=int(event['quantity'])
                if kind==2:active[key]=(event['price_ticks'],qty)
                elif kind==1:
                    maker=(event['symbol'],event['maker']);price,remaining=active[maker];remaining-=qty
                    if remaining:active[maker]=(price,remaining)
                    else:del active[maker]
                elif kind==3:del active[key]
                peak_active=max(peak_active,len(active));peak_levels=max(peak_levels,len({(k[0],v[0]) for k,v in active.items()}))
            subprocess.run([str(binary),'bench','--input',str(corpus),'--engine','both','--output',str(bench)],check=True)
            metadata={'date':datetime.now(ZoneInfo('Europe/Istanbul')).isoformat(),'cpu':cpu,'os':platform.platform(),'compiler_probe':version,'stdlib':stdlib,'flags':flags,'seed':42,'scenario':scenario,'commands':count,'requested_levels_per_side':levels,'peak_active_orders':peak_active,'peak_price_levels':peak_levels,'events_sha256':sha(events),'snapshot_sha256':sha(snapshot),'corpus_sha256':sha(corpus),'binary_sha256':sha(binary),'source_sha256':source.hexdigest(),'revision':revision,'warmups':1,'repetitions':5,'latency_samples_per_repeat':count,'clock':'steady_clock','quantile':'nearest rank ceil(q*N)-1','pinning':False,'profiling':False,'observation':'results retained during timing, checksums and snapshots consumed afterwards; latency results consumed afterwards','limits':'single host, no timer subtraction, clock pair baseline in timer_ns, cache and OS jitter uncontrolled'}
            bench.with_suffix('.meta.json').write_text(json.dumps(metadata,indent=2)+'\n')
            subprocess.run(['python3',str(ROOT/'tools/report.py'),'--bench',str(bench),'--output',str(out/(name+'.html'))],check=True)
if __name__=='__main__': main()
