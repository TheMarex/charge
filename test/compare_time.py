#!/usr/bin/env python3

import pandas as pd
import sys

lhs_df = pd.read_csv(sys.argv[1])
rhs_df = pd.read_csv(sys.argv[2])

print(lhs_df["avg_time"].mean() / 10e6, rhs_df["avg_time"].mean() / 10e6, (lhs_df["avg_time"] / rhs_df["avg_time"]).mean(), (lhs_df["avg_time"].mean() / rhs_df["avg_time"].mean()))



