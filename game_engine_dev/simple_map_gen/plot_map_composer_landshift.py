import sys

import matplotlib.pyplot as plt
import numpy as np

path = sys.argv[1]
d = np.genfromtxt(path, names=True)
x = d["arg"]
for name in ("ocean", "sea", "coastal", "plains", "hills", "mountains"):
    plt.plot(x, d[name], label=name)
plt.xlabel("coastal_lim_01 (target m_terr_lim_coastal)")
plt.ylabel("m_terr_lim_*")
plt.legend()
plt.show()
