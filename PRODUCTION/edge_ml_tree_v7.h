#pragma once
// SIH deployment Decision Tree v7.
// Trained only on the 48 development source files; 12 source files were held out before threshold selection.
// Target: temperature crosses >8C within next 120 minutes, evaluated only when current temperature is <8C.
// Features must match the 10-minute cadence described by the project.
static inline float edge_ml_breach_probability_v7(const float f[10]) {
if (f[7] <= 2.75000000f) {
  if (f[5] <= 0.15000000f) {
    if (f[1] <= 7.12999988f) {
      if (f[9] <= -0.00375000f) {
        if (f[7] <= 1.75000000f) {
          if (f[3] <= -0.02625000f) {
            return 0.00000000f;
          } else {
            return 0.95964400f;
          }
        } else {
          if (f[1] <= 6.33000016f) {
            return 0.63384338f;
          } else {
            return 0.00000000f;
          }
        }
      } else {
        if (f[2] <= 0.29049638f) {
          if (f[3] <= 0.00125000f) {
            return 0.16597865f;
          } else {
            return 0.37680153f;
          }
        } else {
          if (f[9] <= 0.00125000f) {
            return 0.67140201f;
          } else {
            return 0.84612304f;
          }
        }
      }
    } else {
      if (f[6] <= 0.12500000f) {
        if (f[9] <= -0.00125000f) {
          if (f[2] <= 0.99297366f) {
            return 0.85642042f;
          } else {
            return 0.25299993f;
          }
        } else {
          if (f[1] <= 7.68999982f) {
            return 0.31582724f;
          } else {
            return 0.67687132f;
          }
        }
      } else {
        if (f[7] <= 0.45000000f) {
          if (f[2] <= 0.05611767f) {
            return 0.94868597f;
          } else {
            return 0.98698272f;
          }
        } else {
          if (f[2] <= 0.06903935f) {
            return 0.72405727f;
          } else {
            return 0.87466132f;
          }
        }
      }
    }
  } else {
    if (f[7] <= 2.04999995f) {
      if (f[6] <= 0.87500000f) {
        if (f[0] <= 7.04999995f) {
          if (f[9] <= -0.00375000f) {
            return 0.93779964f;
          } else {
            return 0.64551547f;
          }
        } else {
          if (f[1] <= 7.16999984f) {
            return 0.95294901f;
          } else {
            return 0.99328889f;
          }
        }
      } else {
        if (f[3] <= 0.01125000f) {
          if (f[7] <= 1.55000001f) {
            return 0.98162303f;
          } else {
            return 0.00000000f;
          }
        } else {
          if (f[9] <= 0.00625000f) {
            return 0.99585306f;
          } else {
            return 0.93968508f;
          }
        }
      }
    } else {
      if (f[4] <= 0.15000000f) {
        if (f[9] <= -0.00125000f) {
          if (f[1] <= 5.66999984f) {
            return 0.45715399f;
          } else {
            return 0.87380568f;
          }
        } else {
          return 0.00000000f;
        }
      } else {
        if (f[4] <= 0.25000001f) {
          if (f[1] <= 5.27000022f) {
            return 0.82640237f;
          } else {
            return 0.92330593f;
          }
        } else {
          if (f[5] <= 0.75000000f) {
            return 0.97759004f;
          } else {
            return 0.83853298f;
          }
        }
      }
    }
  }
} else {
  if (f[2] <= 0.14831784f) {
    if (f[6] <= 0.12500000f) {
      if (f[1] <= 1.21000004f) {
        if (f[7] <= 6.95000005f) {
          if (f[1] <= 1.11000001f) {
            return 0.47698178f;
          } else {
            return 0.14089072f;
          }
        } else {
          return 0.00000000f;
        }
      } else {
        if (f[1] <= 4.70999980f) {
          if (f[7] <= 4.14999986f) {
            return 0.00000000f;
          } else {
            return 0.06710509f;
          }
        } else {
          if (f[1] <= 4.81000018f) {
            return 0.42359423f;
          } else {
            return 0.04677454f;
          }
        }
      }
    } else {
      if (f[9] <= -0.00625000f) {
        if (f[1] <= 4.56999993f) {
          if (f[6] <= 0.37500000f) {
            return 0.00000000f;
          } else {
            return 0.00000000f;
          }
        } else {
          return 0.94538718f;
        }
      } else {
        if (f[7] <= 4.54999995f) {
          if (f[1] <= 4.79000020f) {
            return 0.02841724f;
          } else {
            return 0.34451092f;
          }
        } else {
          if (f[0] <= 3.04999995f) {
            return 0.22280677f;
          } else {
            return 0.53575776f;
          }
        }
      }
    }
  } else {
    if (f[1] <= 4.62999988f) {
      if (f[2] <= 0.32984623f) {
        if (f[0] <= 1.65000004f) {
          if (f[1] <= 1.52999997f) {
            return 0.00000000f;
          } else {
            return 0.83853298f;
          }
        } else {
          if (f[7] <= 3.54999995f) {
            return 0.29351450f;
          } else {
            return 0.04748739f;
          }
        }
      } else {
        if (f[5] <= 0.15000000f) {
          if (f[3] <= -0.01375000f) {
            return 0.57436270f;
          } else {
            return 0.00000000f;
          }
        } else {
          if (f[9] <= -0.02875000f) {
            return 0.43181261f;
          } else {
            return 0.81924207f;
          }
        }
      }
    } else {
      if (f[6] <= 0.12500000f) {
        if (f[7] <= 3.75000000f) {
          if (f[2] <= 0.46518317f) {
            return 0.56326267f;
          } else {
            return 0.00000000f;
          }
        } else {
          if (f[2] <= 1.23726779f) {
            return 0.76950535f;
          } else {
            return 0.91754834f;
          }
        }
      } else {
        if (f[3] <= 0.01375000f) {
          if (f[1] <= 4.79000020f) {
            return 0.81655844f;
          } else {
            return 0.98133677f;
          }
        } else {
          if (f[0] <= 5.04999995f) {
            return 0.00000000f;
          } else {
            return 0.00000000f;
          }
        }
      }
    }
  }
}
}
