#!/bin/python3

# Emilie Deprez, 2022

from inginious import input, feedback
import json

def get_index(random):
    return int(str(random)[2:4])


def get_mapping(data, index):
    d = {}
    for key in data[index].keys():
        d[data[index][key]] = key
    return d

def checkFormat(string, mapping):
    answer = string.lower().replace(" ", "")
    try:
        return mapping[answer]
    except:
        return None


def check(correct, point, tag, mapping):
    answer = checkFormat(input.get_input(tag), mapping)
    if answer == None:
        feedback.set_problem_result("failed", tag)
        feedback.set_problem_feedback("Wrong input format. Please provide a valid nexthop or interface.", tag)
        return 0
    elif answer != correct:
        feedback.set_problem_result("failed", tag)
        feedback.set_problem_feedback("Remember the binary representation of this address and that IPv6 routers forward packets according to the longest match between the destination address of the packet and the routes in the forwarding table.", tag)
        return 0
    else:
        feedback.set_problem_result("success", tag)
        feedback.set_problem_feedback("Correct", tag)
        return point


# get all possible combinaisons
f = open("public/data.js", "r")
json_text = f.readlines()[0]
f.close()
json_text = json_text.replace("var data = ", "")
json_text = json_text.replace(";", "")
data = json.loads(json_text)

grade = 0
index = get_index(input.get_input("@random")[0])
mapping = get_mapping(data, index)

# Forwarding table
# ::/0 via 1
# 2001:db8::/32 via 2
# 2001:db8:12/46 via 3
# 2001:db8:12/47 via 4
# 2001:db8:12/48 via 5
# 2001:db8:14/48 via 6
# 2001:db8:12:1a4e::/60 via 7
# 2001:db8:12:1a4e::/61 via 8
# 2001:db8:12:1a4e::/63 via 9
# 2001:db8:12:1a4e::/64 via 10


# 2001:db8:13::bc -> 4
# 2001:db8:12:1a4d::476 -> 8
# 2001:db8:16::de -> 2
# 2001:db8:12:1a4f::5 -> 9
# 2001:db8:10::bad:cafe -> 3
# 2001:db8:14:1431::2 -> 6
# 2001:db8:12:1a40::26 -> 7
# 2001:db9:: -> 1

grade += check("4", 12.5, "q1", mapping)
grade += check("8", 12.5, "q2", mapping)
grade += check("2", 12.5, "q3", mapping)
grade += check("9", 12.5, "q4", mapping)
grade += check("3", 12.5, "q5", mapping)
grade += check("6", 12.5, "q6", mapping)
grade += check("7", 12.5, "q7", mapping)
grade += check("1", 12.5, "q8", mapping)


feedback.set_grade(grade)
if grade == 100:
    feedback.set_global_result("success")
else:
    feedback.set_global_result("failed")
