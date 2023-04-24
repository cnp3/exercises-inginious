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
    answer = string.replace("[", "").replace("]", "").replace(" ", "").split(",")
    if len(answer) != 3: return {}
    r = answer[0].upper(); c=answer[1].upper(); t=answer[2].upper()
    if len(r) < 3 or (r[0:2] != "R="): return {}
    if len(c) < 3 or (c[0:2] != "C="): return {}
    if len(t) < 3 or (t[0:2] != "T="): return {}
    if r[2] != "S": r = r[0:2] + "S" + r[2:]
    if t[2] != "S": t = t[0:2] + "S" + t[2:]
    try:
        d = {}
        d["R"] = mapping[r[2:]]
        d["C"] = int(c[2:])
        d["T"] = mapping[t[2:]]
        return d
    except:
        return {}

def check(student, correct, point, tag):
    error = False
    error_msg = []
    if student["R"] != correct["R"]:
        error_msg.append("Incorrect root")
        error = True
    if student["C"] != correct["C"]:
        error_msg.append("Incorrect cost")
        error = True
    if student["T"] != correct["T"]:
        error_msg.append("Incorrect identifier of the switch")
        error = True
    if error:
        feedback.set_problem_result("failed", tag)
        feedback.set_problem_feedback("; ".join(error_msg), tag)
        return 0
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

# set variables

grade = 0
index = get_index(input.get_input("@random")[0])
mapping = get_mapping(data, index)
correct = {"q1":{"R":"Sa", "C":0, "T":"Sa"}, "q2":{"R":"Sa", "C":4, "T":"Sb"}, "q3":{"R":"Sa", "C":6, "T":"Sc"}, "q4":{"R":"Sa", "C":1, "T":"Sd"}, "q5":{"R":"Sa", "C":3, "T":"Se"}}

for tag in ["q1", "q2", "q3", "q4", "q5"]:
    answer = checkFormat(input.get_input(tag), mapping)
    if len(answer) != 3: 
        feedback.set_problem_result("failed", tag)
        feedback.set_problem_feedback("Wrong input format. Please provide an answer with the format R=123,C=45,T=678 (Don't put the port number)", tag)
    else:
        grade += check(answer, correct[tag], 20, tag)


# set grade and global result
feedback.set_grade(grade)
if int(grade) == 100:
    feedback.set_global_result("success")
else:
    feedback.set_global_result("failed")