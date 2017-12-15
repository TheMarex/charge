import gdb.printing
import re

def iterate(v):
    s, e = v['_M_impl']['_M_start'], v['_M_impl']['_M_finish']
    while s != e:
        yield s.dereference()
        s +=1

class SweeplineEventPrinter:
    """Print a SweeplineEvent object."""
    def __init__(self, val):
        self.val = val

    def to_string(self):
        t = self.val["type"]
        if t == 1:
            e = self.val["begin_event"]
            return "{BeginEvent x=%(x)f y=%(y)f segment_index=%(segment_index)i }" % e
        elif t == 0:
            e = self.val["end_event"]
            return "{EndEvent x=%(x)f y=%(y)f segment_index=%(segment_index)i }" % e
        elif t == 2:
            e = self.val["intersection_event"]
            return "{IntersectionEvent x=%(x)f y=%(y)f first_index=%(first_index)i second_index=%(second_index)i}" % e
        else:
            return "{?}"

class HypOrLinFunctionPrinter:
    """Print a SweeplineEvent object."""
    def __init__(self, val):
        self.val = val

    def to_string(self):
        d = self.val["lin"]["d"]
        if d <= 0:
            lin = self.val["lin"]
            return "{LinearFunction d = %(d)f b = %(b)f c = %(c)f }" % lin
        else:
            hyp = self.val["hyp"]
            return "{HyperbolicFunction a = %(a)f b = %(b)f c = %(c)f }" % hyp

# Prints a object as suitable for the python code
class PythonPrinter (gdb.Command):
    def __init__ (self):
        super (PythonPrinter, self).__init__ ('ppy', gdb.COMMAND_USER)
        self.priority = [
                'std::vector<charge::common::LabelEntryWithParent<charge::common::LimitedFunction<',
                'std::vector<charge::common::LabelEntryWithParent<std::tuple<int, int>',
                '::StatefulFunction<charge::common::PiecewieseFunction',
                '::PiecewieseFunction<charge::common::HypOrLinFunction',
                '::cost_t',
                '::PiecewieseFunction<charge::common::LinearFunction',
                '::PiecewieseTradeoffFunction',
                '::PiecewieseDecHypOrLinFunction',
                '::PiecewieseDecLinearFunction',
                '::PiecewieseFunction',
                '::LimitedHypOrLinFunction',
                '::LimitedFunction',
                '::LinearFunction',
                '::HypOrLinFunction',
                '::HyperbolicFunction']
        self.to_python = {
                'std::vector<charge::common::LabelEntryWithParent<charge::common::LimitedFunction<': self.label_vector,
                'std::vector<charge::common::LabelEntryWithParent<std::tuple<int, int>': self.tuple_label_vector,
                '::StatefulFunction<charge::common::PiecewieseFunction': self.stateful_function,
                '::PiecewieseFunction<charge::common::HypOrLinFunction': self.piecewise_function,
                '::cost_t': self.piecewise_function,
                '::PiecewieseFunction<charge::common::LinearFunction': self.piecewise_function_dec_linear,
                '::PiecewieseTradeoffFunction': self.piecewise_function,
                '::PiecewieseDecHypOrLinFunction': self.piecewise_function,
                '::PiecewieseDecLinearFunction': self.piecewise_function_dec_linear,
                '::PiecewieseFunction': self.piecewise_function,
                '::LinearFunction': self.linear_function,
                '::LimitedHypOrLinFunction': self.limited_function,
                '::LimitedFunction': self.limited_function,
                '::HypOrLinFunction': self.hyp_lin_function,
                '::HyperbolicFunction': self.hyperbolic_function}

    @staticmethod
    def label_vector(vector):
        labels = []
        for l in iterate(vector):
            labels.append(PythonPrinter.piecewise_function(l["cost"]))
        return "[" + ",".join(labels) + "]"

    @staticmethod
    def tuple_label_vector(vector):
        labels = []
        for l in iterate(vector):
            first = l["cost"]
            second = first.cast(gdb.lookup_type("std::_Tuple_impl<1, int>"))
            labels.append("({},{})".format(first["_M_head_impl"], second["_M_head_impl"]))
        return "[" + ",".join(labels) + "]"

    @staticmethod
    def linear_function(lin):
        return "LinearFunction(d = %(d)f, b = %(b)f, c = %(c)f)" % lin

    @staticmethod
    def hyperbolic_function(hyp):
        return "HypLinFunction(a = %(a)f, b = %(b)f, c = %(c)f, d = 0)" % hyp

    @staticmethod
    def hyp_lin_function(val):
        d = val["lin"]["d"]
        if d <= 0:
            return PythonPrinter.linear_function(val["lin"])
        else:
            return PythonPrinter.hyperbolic_function(val["hyp"])

    @staticmethod
    def limited_function(f, sub_fn=None):
        sub_fn = sub_fn or PythonPrinter.hyp_lin_function
        s = "PiecewiseFunction([0.0, %(min_x)f, %(max_x)f], [LinearFunction(d=0, b=0, c=float('inf')), %%s])" % f
        return s % sub_fn(f["function"])

    @staticmethod
    def stateful_function(f, sub_fn=None):
        sub_fn = sub_fn or PythonPrinter.piecewise_function_dec_linear
        return sub_fn(f["function"])

    @staticmethod
    def piecewise_function_dec_linear(pwf):
        return PythonPrinter.piecewise_function(pwf, PythonPrinter.linear_function)

    @staticmethod
    def piecewise_function(pwf, sub_fn=None):
        sub_fn = sub_fn or PythonPrinter.hyp_lin_function
        fns = ["LinearFunction(0, float('inf'))"]
        xs = [0.0]
        last = None
        for f in iterate(pwf["functions"]):
            xs.append(float(f["min_x"]))
            fns.append(sub_fn(f["function"]))
            last = f
        if last is None:
            s = "PiecewiseFunction([], [])"
        else:
            xs.append(float(last["max_x"]))
            xs = [str(x) for x in xs]
            s =  "PiecewiseFunction([" + ",".join(xs) + "], [" + ",".join(fns) + "])"
        return s

    def invoke (self, arg, from_tty):
        val = gdb.parse_and_eval(arg)
        for key in self.priority:
            #print(str(val.type), key)
            if re.search(key, str(val.type)) is not None:
                print(self.to_python[key](val))
                return
        print ('no Python printer for: ' + str(val.type))

# Prints a object as suitable for the c++ code
class CxxPrinter (gdb.Command):
    def __init__ (self):
        super (CxxPrinter, self).__init__ ('pcxx', gdb.COMMAND_USER)
        self.priority = ['charge::common::PiecewieseFunction<charge::common::LinearFunction',
                         'charge::common::PiecewieseFunction',
                         'charge::ev::PiecewieseTradeoffFunction',
                         'charge::common::PiecewieseDecHypOrLinFunction',
                         '::cost_t',
                         'charge::common::LimitedFunction',
                         'charge::common::LinearFunction',
                         'charge::common::HypOrLinFunction',
                         'charge::common::HyperbolicFunction']
        self.to_cxx = {
                'charge::common::PiecewieseFunction<charge::common::LinearFunction': self.piecewise_lin_function,
                'charge::common::PiecewieseFunction': self.piecewise_function,
                'charge::ev::PiecewieseTradeoffFunction': self.piecewise_function,
                'charge::common::PiecewieseDecHypOrLinFunction': self.piecewise_function,
                '::cost_t': self.piecewise_function,
                'charge::common::LimitedFunction': self.limited_function,
                'charge::common::LinearFunction': self.linear_function,
                'charge::common::HypOrLinFunction': self.hyp_lin_function,
                'charge::common::HyperbolicFunction': self.hyperbolic_function}

    @staticmethod
    def linear_function(lin):
        return "LinearFunction{%(d)f, %(b)f, %(c)f}" % lin

    @staticmethod
    def hyperbolic_function(hyp):
        return "HyperbolicFunction{%(a)f, %(b)f, %(c)f}" % hyp

    @staticmethod
    def hyp_lin_function(val):
        d = val["lin"]["d"]
        if d <= 0:
            return CxxPrinter.linear_function(val["lin"])
        else:
            return CxxPrinter.hyperbolic_function(val["hyp"])

    @staticmethod
    def limited_function(l, sub_fn=None):
        sub_fn = sub_fn or CxxPrinter.hyp_lin_function
        return "{%(min_x)f, %(max_x)f, %(f)s}" % {"min_x": l["min_x"], "max_x": l["max_x"], "f": sub_fn(l["function"])}

    @staticmethod
    def piecewise_function(pwf, sub_fn=None):
        sub_fn = sub_fn or CxxPrinter.hyp_lin_function
        fns = []
        for f in iterate(pwf["functions"]):
            fns.append(CxxPrinter.limited_function(f, sub_fn))
        return "{{" + ",".join(fns) + "}}"

    @staticmethod
    def piecewise_lin_function(pwf):
        return CxxPrinter.piecewise_function(pwf, CxxPrinter.linear_function)

    def invoke (self, arg, from_tty):
        val = gdb.parse_and_eval(arg)
        for key in self.priority:
            if re.search(key, str(val.type)) is not None:
                print(self.to_cxx[key](val))
                return
        print ('no Cxx printer for: ' + str(val.type))

class PathLabelPrinter(gdb.Command):
    def __init__(self, printer):
        super (PathLabelPrinter, self).__init__ ('ppl', gdb.COMMAND_USER)
        self.__printer = printer

    def invoke(self, arg, from_tty):
        argv = gdb.string_to_argv(arg)
        path = gdb.parse_and_eval(argv[0])
        labels = argv[1]
        print("[")
        for node in iterate(path):
            print("({}, ".format(node), end='')
            self.__printer.invoke(labels + ".settled_labels[{}]".format(node), False)
            print(")")
        print("]")

ppy = PythonPrinter()
pcxx = CxxPrinter()
ppl = PathLabelPrinter(ppy)

def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter('Charge')
    pp.add_printer('SweeplineEvent', '::detail::SweeplineEvent$', SweeplineEventPrinter)
    pp.add_printer('HypOrLinFunction', '::HypOrLinFunction$', HypOrLinFunctionPrinter)
    return pp

gdb.pretty_printers = [x for x in gdb.pretty_printers if x.name != 'Charge']
gdb.printing.register_pretty_printer(gdb.current_objfile(), build_pretty_printer())
