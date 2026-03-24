#!/usr/bin/env python3
"""LLM latency race: time parallel API calls to N providers."""
import subprocess as S,time,json;from concurrent.futures import ThreadPoolExecutor as T
MODELS={"claude":"claude -p 'say hi'","gemini":"gemini -p 'say hi'"}
def bench(n,c):t=time.perf_counter();S.run(c,shell=1,capture_output=1);return n,time.perf_counter()-t
with T(len(MODELS))as e:
    for n,t in sorted(e.map(lambda kv:bench(*kv),MODELS.items()),key=lambda x:x[1]):
        print(f"{t*1000:7.0f}ms {n}")
