import importlib.util,pathlib,tempfile,csv,sys
spec=importlib.util.spec_from_file_location('report',sys.argv[1]);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
with tempfile.TemporaryDirectory() as t:
 p=pathlib.Path(t);b=p/'bench.csv';o=p/'report.html';b.write_text('engine,commands\n')
 try:m.render(b,o)
 except ValueError:pass
 else:raise RuntimeError('empty data accepted')
 keys=['engine','commands','throughput_ops_s','p50_ns','p95_ns','p99_ns','max_ns','timer_ns']
 with b.open('w') as f:
  w=csv.DictWriter(f,fieldnames=keys);w.writeheader();w.writerow(dict(zip(keys,['<script>x</script>',10,100,10,20,30,40,5])))
 m.render(b,o);s=o.read_text();assert '<script>' not in s and '&lt;script&gt;' in s
print('Report rejects empty data and escapes untrusted text')
