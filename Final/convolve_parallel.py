import numpy as np

k=[1,2]
a=list(range(10))

print('ref\t',np.convolve(k,a,mode='full'))
win = 4

def frac(a,k):
    buf = np.empty(len(k)+len(a)-1)
    idx =0
    while idx < len(a):
        t = np.convolve(k,a[idx:idx+win],mode='full')
        print('partial\t',t)
        buf[idx:idx+win+1] += t
        idx += win
    print('result\t',buf)

frac(a,k)
