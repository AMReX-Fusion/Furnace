#!/bin/env python

import numpy as np
import matplotlib.pyplot as plt

import yt

low = yt.load("flame_wave_128_plt01600")
med = yt.load("flame_wave_256_plt03200")
high = yt.load("flame_wave_512_plt06400")

fig, ax = plt.subplots(1,1)

for ds, l in [(low, "128"), (med, "256"), (high, "512")]:

    ray = ds.ray((0, 0, 0), (2.56e3, 0, 0))

    isrt = np.argsort(ray["x"])

    ax.plot(ray["x"][isrt], ray["magvel"][isrt], label=l)

fig.legend(frameon=False)
ax.set_yscale("log")
fig.tight_layout()

fig.savefig("hse.png")