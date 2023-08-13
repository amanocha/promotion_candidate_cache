import numpy as np
import matplotlib.pyplot as plt
import os
import re
import sys

AXIS_FONTSIZE = 28
TICK_FONTSIZE = 24
INPUTS_FONTSIZE = 24

scale = 1.25
width = 0.95

def plot():
    size = (18.0, 6.0)
    fig = plt.figure(figsize=size)
    fig.subplots_adjust(bottom=0.1)
    ax1 = fig.add_subplot(111)

    line1 = ax1.plot(x1, y1, color='blue', marker='o', label='pagerank', linewidth=5, markersize=10)
    line2 = ax1.plot(x2, y2, color='red', marker='o', label='mcf', linewidth=8, markersize=16)