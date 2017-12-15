import math

eps = 0.0001

def eps_round(value):
    if math.isinf(value):
        return value
    return round(value, math.floor(-math.log(eps)/math.log(10)))

