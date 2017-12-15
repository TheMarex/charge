import pandas as pd
import json
import math
import io
import os.path
import gzip
import bz2

def get_stops(r):
    path = r["routes"][0]["path"]
    durations = r["routes"][0]["durations"]
    consumptions = r["routes"][0]["consumptions"]
    stops = []
    for i in range(len(path)-1):
        if path[i] == path[i+1]:
            dt = durations[i+1] - durations[i]
            dc = consumptions[i] - consumptions[i+1]
            stops.append({'time': dt, 'entry_consumption': consumptions[i], 'exit_consumption': consumptions[i+1]})
    return stops

def get_speed_changes(r):
    durations = r["routes"][0]["durations"]
    lengths = r["routes"][0]["lengths"]
    speed_changes = 0
    prev_speed = 0
    prev_length = lengths[0]
    prev_duration = durations[0]
    for i in range(len(durations)-1):
        dt = durations[i+1] - prev_duration
        dl = lengths[i+1] - prev_length
        if dt > 0 and dl > 5:
            speed = int(dl / dt * 3.6)
            if math.fabs(prev_speed - speed) >= 5:
                speed_changes += 1
            prev_length = lengths[i+1]
            prev_duration = durations[i+1]
            prev_speed = speed
    return speed_changes

def read_ranks(path, dfs):
    ranks_df = pd.read_csv(path)
    merged_dfs = []
    for df in dfs:
        merged_dfs.append(df.merge(ranks_df, on=['start', 'target']))
    return merged_dfs

def get_route_tradeoff(path):
    pass

def read_data(data_path):
    """
    Merged .json and csv based result data to one DataFrame
    """
    if not os.path.isfile(data_path):
        if os.path.isfile(data_path + ".gz"):
            data_path += ".gz"
        elif os.path.isfile(data_path + ".bz2"):
            data_path += ".bz2"

    if data_path.endswith(".gz"):
        with gzip.open(data_path) as binary_file:
            file = io.TextIOWrapper(binary_file)
            lines = file.readlines()
            data = [json.loads(l) for l in lines]
    elif data_path.endswith(".bz2"):
        with bz2.open(data_path) as binary_file:
            file = io.TextIOWrapper(binary_file)
            lines = file.readlines()
            data = [json.loads(l) for l in lines]
    else:
        with open(data_path, "r") as file:
            lines = file.readlines()
            data = [json.loads(l) for l in lines]
    return data

def get_query(data, start, target):
    for d in data:
        if int(d['start']) == start and int(d['target']) == target:
            return d
    return None

def get_query_data(r):
    route = r['routes'][0]
    durations = route['durations']
    lengths = route['lengths']
    consumptions = route['consumptions']
    heights = route['heights']
    speeds = [0]
    columns = {'durations': durations, 'speeds': speeds, 'consumptions': consumptions, 'heights': heights, 'lengths': lengths}
    for i in range(len(durations)-1):
        d = durations[i+1]-durations[i]
        l = lengths[i+1]-lengths[i]
        if d == 0:
            speeds.append(float('nan'))
        else:
            speeds.append(l/d * 3.6)

    if 'max_speeds' in route:
        columns['max_speeds'] = route['max_speeds']

    df = pd.DataFrame(columns)
    return df

def get_data(path):
    """
    Merged .json and csv based result data to one DataFrame
    """
    data = read_data(path + '.json')

    columns = {'start': [], 'target': [], 'consumption': [], 'duration': [], 'stops': [], 'sum_charging_time': [], 'sum_charging_consumption': [], 'avg_speed': [], 'path': [], 'speed_changes': []}
    for r in data:
        if r is None:
            continue
        stops = get_stops(r)
        speed_changes = get_speed_changes(r)
        avg_speed = sum(r['routes'][0]['lengths']) / sum(r['routes'][0]['durations'])
        columns['start'].append(int(r["start"]))
        columns['target'].append(int(r["target"]))
        columns['duration'].append(r["routes"][0]["durations"][-1])
        columns['consumption'].append(r["routes"][0]["consumptions"][-1])
        columns['stops'].append(len(stops))
        columns['speed_changes'].append(speed_changes)
        columns['sum_charging_time'].append(sum([s['time'] for s in stops]))
        columns['sum_charging_consumption'].append(sum([s['entry_consumption'] - s['exit_consumption'] for s in stops]))
        columns['avg_speed'].append(avg_speed)
        columns['path'].append(len(r['routes'][0]["path"]))
    result_df = pd.DataFrame(columns)

    df = pd.read_csv(path)
    merged_df = df.merge(result_df, on=['start', 'target'])
    merged_df.loc[merged_df["avg_time"] < 0, 'avg_time'] += 2**32
    merged_df["avg_time"] = merged_df["avg_time"] / 1000.0 / 1000.0
    return merged_df

def get_consumption_data(path):
    """
    Merged .json and csv based result data to one DataFrame
    """
    columns = {'start': [], 'target': [], 'id': [], 'time':[], 'entry_consumption': [], 'exit_consumption': []}
    with open(path + ".json", "r") as file:
        lines = file.readlines()
        data = [json.loads(l) for l in lines]
        for r in data:
            if r is None:
                continue
            stops = get_stops(r)
            columns['start'].append(r["start"])
            columns['target'].append(r["target"])
            columns['id'].append(0)
            columns['time'].append(float('nan'))
            columns['entry_consumption'].append(float('nan'))
            columns['exit_consumption'].append(float('nan'))
            for i, s in enumerate(stops):
                columns['start'].append(r["start"])
                columns['target'].append(r["target"])
                columns['id'].append(i+1)
                columns['time'].append(s['time'])
                columns['entry_consumption'].append(s['entry_consumption'])
                columns['exit_consumption'].append(s['exit_consumption'])

    return pd.DataFrame(columns)

def compare_column(column, dfs):
    """
    Creates a comparition df from the given column
    """
    df = None
    for key in dfs:
        tmp_df = dfs[key][["start", "target", column]]
        tmp_df = tmp_df.rename(columns={column: key})
        if df is None:
            df = tmp_df
        else:
            df = df.merge(tmp_df, on=['start', 'target'])

    return df

def df_to_latex_table(df, columns=None):
    """
    Returns a latex table from columsn of the df
    """
    columns = columns or list(df.columns)
    template = """
    \\begin{tabular}{%s}
        %s
        \hline
        %s
    \\end{tabular}
    """
    formating = "|".join(["c"]*len(columns))
    header = " & ".join(columns) + "//"
    rows = []
    for index, row in df.iterrows():
        values = []
        for c in columns:
            values.append(str(row[c]))
        row = " & ".join(values)
        rows.append(row)
    rows = "//\n".join(rows)

    return template % (formating, header, rows)

def fill_latex_table(path, tables):
    out = path.replace(".tex.in", ".tex")
    with open(path) as f:
        template = f.read()
    tokens = {}
    for name in tables:
        for key in tables[name].columns:
            value = tables[name][key].mean()
            value_max = tables[name][key].max()
            token = "\external{%s_%s}" % (name, key)
            if math.isfinite(value):
                tokens[token] = "{:,}".format(int(value)).replace(",", " ")
            else:
                tokens[token] = "n/a"
            token = "\externalmax{%s_%s}" % (name, key)
            if math.isfinite(value):
                tokens[token] = "{:,}".format(int(value_max)).replace(",", " ")
            else:
                tokens[token] = "n/a"
            token = "\externalf{%s_%s}" % (name, key)
            if math.isfinite(value):
                tokens[token] = "%.02f" % value
            else:
                tokens[token] = "n/a"
            token = "\externalmaxf{%s_%s}" % (name, key)
            if math.isfinite(value):
                tokens[token] = "%.02f" % value_max
            else:
                tokens[token] = "n/a"
            token = "\externalf[3]{%s_%s}" % (name, key)
            if math.isfinite(value):
                tokens[token] = "%.03f" % value
            else:
                tokens[token] = "n/a"
            token = "\externalmaxf[3]{%s_%s}" % (name, key)
            if math.isfinite(value_max):
                tokens[token] = "%.03f" % value_max
            else:
                tokens[token] = "n/a"

    for t in tokens:
        template = template.replace(t, tokens[t])
    with open(out, "w+") as f:
        f.write(template)

