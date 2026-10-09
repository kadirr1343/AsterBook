import pathlib, subprocess, sys, tempfile
exe, example = sys.argv[1:]
def run(args, code=0):
    r = subprocess.run([exe, *args], capture_output=True, text=True)
    if r.returncode != code:
        raise RuntimeError(f"{args}: {r.returncode} != {code}: {r.stderr}")
    return r.stdout
with tempfile.TemporaryDirectory() as tmp:
    p = pathlib.Path(tmp)
    run(['--help']);run(['wat'], 2);run(['demo','--wat','x'], 2)
    run(['replay','--input', example], 2)
    run(['verify','--input', str(p/'missing')], 3)
    for engine in ['reference','price-level']:
        run(['replay','--input',example,'--engine',engine,'--events',str(p/engine/'events.csv'),'--snapshot',str(p/engine/'book.csv')])
    for file in ['events.csv','book.csv']:
        assert (p/'reference'/file).read_bytes() == (p/'price-level'/file).read_bytes()
    demo=run(['demo'])
    events=(p/'reference/events.csv').read_text();book=(p/'reference/book.csv').read_text()
    assert demo.endswith(events+book)
    rows=events.splitlines()
    assert '5,1,1,0,0,0,0,0,0,3' in rows  # duplicate
    assert '1,1,4,1,4,1,1,100,5,0' in rows
    assert '1,1,4,2,4,2,2,100,7,0' in rows
    assert '1,1,4,3,4,3,3,101,2,0' in rows
    for scenario in ['add-only','cancel-heavy','mixed','sweep-heavy']:
        corpus=p/(scenario+'.csv')
        run(['generate','--seed','42','--count','100','--scenario',scenario,'--output',str(corpus)])
        run(['verify','--input',str(corpus)])
    run(['bench','--input',str(corpus),'--engine','both','--output',str(p/'bench.csv')])
    header='seq,action,symbol,order_id,side,price_ticks,quantity,tif\n'
    for bad in ['1,ADD,1,1,BUY,-1,1,GTC\n','1,CANCEL,1,1,,,,\n1,CANCEL,1,1,,,,\n','1,CANCEL,1,1,,,,\n\n','"1",CANCEL,1,1,,,,\n']:
        (p/'bad.csv').write_text(header+bad)
        run(['verify','--input',str(p/'bad.csv')],3)
    run(['replay','--input',example,'--engine','price-level','--events',str(p),'--snapshot',str(p/'book.csv')],3)
print('CLI replay, demo golden, generator, benchmark and error paths passed')
