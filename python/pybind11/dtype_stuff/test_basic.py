import numpy as np

from __main__ import Custom
from numpy.core.test_rational import rational

def check_bad():
    av_bad = np.array([Custom(1)])
    print(repr(av_bad))
    print(av_bad == av_bad)

def check_dtype():
    print(np.dtype(Custom))
    print(Custom.dtype)
    print(type(Custom.dtype))

    class Stuff(object): pass
    print(np.dtype(Stuff))
    print(np.dtype(Custom))

def check_op():
    a = Custom(1)
    print(a == a)
    av = np.array([[Custom(1), Custom(2)], [Custom(3), Custom(4)]], dtype=Custom)
    print(av)
    print(av == av)
    print av * av
    print av < av
    print -av
    print np.power(av, av)
    xv = av.astype(float)
    print(repr(xv))
    print(repr(xv.astype(Custom)))
    ov = av.astype(object)
    print(repr(ov))
    print(repr(ov.astype(Custom)))

    a0 = np.zeros(2, 2, Custom)
    print(a0)

def yar():
    x = np.zeros((2, 2), rational)
    print(repr(x))
    y = np.array([rational(1)])
    print(repr(y))

check_bad()
yar()
# check_op()
