import numpy as np

k = [1, 2]
a = list(range(10))

print('full\t', a, '*', k, '=', np.convolve(k, a, mode='full'))


def partialConv(a, k, chunk):
    print('chunk_size =', chunk)
    print('---')
    buf = np.zeros(len(k)+len(a)-1)
    idx = 0
    while idx < len(a):
        t = np.convolve(k, a[idx:idx+chunk], mode='full')
        print('partial\t', a[idx:idx+chunk], '*', k, '=', t)
        buf[idx:idx+chunk+1] += t
        idx += chunk
    print('total\t', buf)


partialConv(a, k, 4)
