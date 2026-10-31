"""Compare bounded original/VR-raster native execution; private owned media only."""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time
from run_diagnostic import command

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def compare(root):
    root=Path(root);a=root/'reference';b=root/'fast'
    counts={}
    for pattern in ('*.bin','native.wav','events.txt','state.tsv'):
        files=sorted(p.name for p in a.glob(pattern))
        if not files or files!=sorted(p.name for p in b.glob(pattern)):
            raise ValueError('Missing/mismatched checkpoint sets: '+pattern)
        for name in files:
            if name=='events.txt':
                # Lua pairs() has no map iteration order. Compare the complete
                # port inventory as a set; timed native execution stays ordered.
                left=(a/name).read_text().splitlines();right=(b/name).read_text().splitlines()
                if sorted(s for s in left if s.startswith('PORT '))!=sorted(s for s in right if s.startswith('PORT ')) or \
                   [s for s in left if not s.startswith('PORT ')]!=[s for s in right if not s.startswith('PORT ')]:
                    raise ValueError('Native event mismatch')
            elif digest(a/name)!=digest(b/name):raise ValueError('Native mismatch: '+name)
        counts[pattern]=len(files)
    mode=json.loads((root/'runs.json').read_text())['mode']
    skips={};fast_skipped=set()
    for run in ('reference','fast'):
        with (root/run/'raster.tsv').open() as log:rows=list(csv.DictReader(log,delimiter='\t'))
        skips[run]=sum(int(row['skip']) for row in rows)
        if run=='fast':fast_skipped={int(row['sequence']) for row in rows if int(row['skip'])}
        if not rows:raise ValueError('No raster decisions')
    if skips['reference']:raise ValueError('Reference bypassed native renderer')
    if mode!='gameplay' and skips['fast']:raise ValueError('Fallback mode bypassed CPU raster')
    if not skips['fast'] and mode=='gameplay':
        raise ValueError('Optimization never exercised')
    scenes=sorted(p.parent.name for p in (a/'scenes').glob('*/metadata.json'))
    if not scenes or scenes!=sorted(p.parent.name for p in (b/'scenes').glob('*/metadata.json')):
        raise ValueError('Scene sets differ')
    fallback=0
    for name in scenes:
        ca=a/'scenes'/name;cb=b/'scenes'/name
        for p in ca.iterdir():
            if p.name=='native-argb32.bin':continue
            if digest(p)!=digest(cb/p.name):raise ValueError('Scene mismatch: '+name+'/'+p.name)
        native_equal=digest(ca/'native-argb32.bin')==digest(cb/'native-argb32.bin')
        sequence=json.loads((ca/'metadata.json').read_text())['sequence']
        if sequence not in fast_skipped and not native_equal:raise ValueError('Fallback raster mismatch: '+name)
        if native_equal:fallback+=1
    return dict(native_identical=counts,raster_skips=skips,identical_owned_scenes=len(scenes),identical_native_fallback_scenes=fallback)

def run(engine,roms,host,output,seconds=60,mode='gameplay',uart=False):
    output=Path(output).resolve()
    if output.exists():raise FileExistsError(output)
    output.mkdir(parents=True);runs={'mode':mode,'uart':uart}
    for variant in ('reference','fast'):
        args,folder=command(engine,roms,output/variant,seconds,Path(__file__).with_suffix('.lua'))
        args+=['-wavwrite',str(folder/'native.wav'),'-numprocessors','2']
        env={k:v for k,v in os.environ.items() if not k.startswith('RCHASE2_')}
        env.update(RCHASE2_VR='1',RCHASE2_FAST_RASTER='1',RCHASE2_XR_LIBRARY=str(Path(host).resolve()),
            RCHASE2_RESEARCH_DIR=str(folder),RCHASE2_DIAGNOSTIC_MODE=mode,
            RCHASE2_RASTER_LOG=str(folder/'raster.tsv'),RCHASE2_XR_LOG=str(folder/'xr.log'),
            RCHASE2_LIVE_PROOF_DIR=str(folder/'scenes'),TMP=str(folder),TEMP=str(folder))
        if uart:env['RCHASE2_UART_COALESCE']='1'
        if variant=='reference':
            env['RCHASE2_CPU_RASTER']='0' # Presence selects original.
            env['RCHASE2_UART_REFERENCE']='0'
            env['RCHASE2_FRAME_POOL']='0'
        (folder/'command.json').write_text(json.dumps(args,indent=2))
        start=time.monotonic()
        with (folder/'engine.log').open('w') as log:
            child=subprocess.Popen(args,cwd=folder,env=env,stdout=log,stderr=subprocess.STDOUT)
            try:code=child.wait(timeout=max(120,seconds*6))
            except subprocess.TimeoutExpired:child.kill();child.wait();raise RuntimeError('Proof timeout')
        if code or '[LUA ERROR]' in (folder/'engine.log').read_text(encoding='utf-8',errors='replace'):
            raise RuntimeError(f'{variant} failed: {code}')
        runs[variant]=dict(exit_code=code,wall_seconds=time.monotonic()-start)
    (output/'runs.json').write_text(json.dumps(runs,indent=2))
    result=compare(output);result['runs']=runs
    (output/'comparison.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('engine','roms','host','output'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--seconds',type=int,default=60)
    parser.add_argument('--mode',choices=('gameplay','service','attract'),default='gameplay')
    parser.add_argument('--uart',action='store_true')
    run(**vars(parser.parse_args()))
