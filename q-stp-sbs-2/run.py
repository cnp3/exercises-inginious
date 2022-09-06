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


def checkFormatSwitch(string, mapping):
    answer = string.replace(" ", "").upper()
    if len(answer) < 1:
        return 0
    if answer[0] != "S": answer = "S" + answer
    try:
        return mapping[answer]
    except:
        return 0


def checkFormatBPDU(string, mapping):
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


def checkFormatPort(string, list_port):
    answer = string.replace("[", "").replace("]", "").replace(" ", "").split(",")
    d = {}
    for a in answer:
        a = a.split(":")
        if len(a) != 2 or a[0] not in list_port or (a[1].lower() != "root" and a[1].lower() != "designated" and a[1].lower() != "blocked"):
            return {}
        d[a[0]] = a[1].lower()
    return d


def checkQRoot(correct, point, tag, mapping):
    answer = checkFormatSwitch(input.get_input(tag), mapping)
    if answer == 0: 
        feedback.set_problem_result("failed", tag)
        feedback.set_problem_feedback("Wrong input format. Please provide an answer with the format Sx where x is the number of the switch", tag)
        return 0
    if answer != correct:
        feedback.set_problem_result("failed", tag)
        feedback.set_problem_feedback("The root switch is the switch with the lowest number.", tag)
        return 0
    else:
        feedback.set_problem_result("success", tag)
        feedback.set_problem_feedback("Correct", tag)
        return point


def checkQBPDU(correct, point, tag, mapping):
    answer = checkFormatBPDU(input.get_input(tag), mapping)
    if len(answer) != 3: 
        feedback.set_problem_result("failed", tag)
        feedback.set_problem_feedback("Wrong input format. Please provide an answer with the format R=123,C=45,T=678 (Don't put the port number)", tag)
        return 0
    error = False
    error_msg = []
    if answer["R"] != correct["R"]:
        error_msg.append("Incorrect root")
        error = True
    if answer["C"] != correct["C"]:
        error_msg.append("Incorrect cost")
        error = True
    if answer["T"] != correct["T"]:
        error_msg.append("Incorrect identifier of the switch")
        error = True
    if error:
        feedback.set_problem_result("failed", tag)
        feedback.set_problem_feedback("; ".join(error_msg), tag)
        return 0
    feedback.set_problem_result("success", tag)
    feedback.set_problem_feedback("Correct", tag)
    return point


def checkQBestBPDU(correct, point, tag, mapping):
    answer = checkFormatSwitch(input.get_input(tag), mapping)
    if answer == 0: 
        feedback.set_problem_result("failed", tag)
        feedback.set_problem_feedback("Wrong input format. Please provide an answer with the format Sx where x is the number of the switch", tag)
        return 0
    if answer != correct:
        feedback.set_problem_result("failed", tag)
        feedback.set_problem_feedback("BPDU1 [R=R1, C=C1, T=T1] is better than BPDU2 [R=R2, C=C2, T=T2] when R1 < R2. If R1 = R2, BPDU1 is better if C1 < C2. If R1 = R2 and C1 = C2 then BPDU1 is better if T1 < T2.", tag)
        return 0
    else:
        feedback.set_problem_result("success", tag)
        feedback.set_problem_feedback("Correct", tag)
        return point


def checkQPortState(correct, point, tag, list_port):
    answer = checkFormatPort(input.get_input(tag), list_port)
    if len(answer) != len(list_port):
        feedback.set_problem_result("failed", tag)
        feedback.set_problem_feedback("Wrong input format. Please provide an answer with the format 1:Root, 2:Designated, 3:Blocked, ...", tag)
        return 0
    error = False
    error_msg = []
    for port in list_port:
        if correct[port] != answer[port]:
            error_msg.append(f"The port {port} is not {answer[port]}")
            error = True
    if error:
        feedback.set_problem_result("failed", tag)
        feedback.set_problem_feedback("; ".join(error_msg) + ". All ports on the root switch are in the designated state. For the other switches, the port with the best incoming BPDU is in a root state. The other ports are either designated or blocked. If the switch's BPDU is better than the one received on a port, the port is in a designated state, otherwise it is in a blocked state.", tag)
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
tag_list = ["q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12", "q13"]


grade += checkQRoot("Sa", 10, "q1", mapping)
if(grade == 10):
    grade += checkQBPDU({"R":"Sa", "C":0, "T":"Sa"}, 10, "q2", mapping)
    grade += checkQBPDU({"R":"Sa", "C":1, "T":"Sc"}, 10, "q3", mapping)
    grade += checkQBPDU({"R":"Sa", "C":2, "T":"Sb"}, 10, "q4", mapping)
    grade += checkQBPDU({"R":"Sa", "C":1, "T":"Sd"}, 10, "q5", mapping)
else:
    for tag in tag_list[1:]:
        feedback.set_problem_result("failed", tag)
        feedback.set_problem_feedback("This question will be corrected when you have answered question 1 correctly.", tag)

if(grade == 50):
    grade += checkQBestBPDU("Sa", 10, "q6" , mapping)
    grade += checkQBestBPDU("Sa", 10, "q7" , mapping)
    grade += checkQBestBPDU("Sc", 10, "q8" , mapping)
    grade += checkQBestBPDU("Sd", 10, "q9" , mapping)
elif(grade >= 10):
    for tag in tag_list[5:]:
        feedback.set_problem_result("failed", tag)
        feedback.set_problem_feedback("This question will be corrected when you have answered questions 2, 3, 4 and 5 correctly.", tag)

if(grade == 90):
    grade += checkQPortState({"1":"designated", "2":"designated"}, 10, "q10", ["1", "2"])
    grade += checkQPortState({"1":"root", "2":"designated"}, 10, "q11", ["1", "2"])
    grade += checkQPortState({"1":"root"}, 10, "q12", ["1"])
    grade += checkQPortState({"1":"blocked", "2":"root", "3":"designated"}, 10, "q13", ["1", "2", "3"])
elif(grade >= 50):
    for tag in tag_list[9:]:
        feedback.set_problem_result("failed", tag)
        feedback.set_problem_feedback("This question will be corrected when you have answered questions 6, 7, 8 and 9 correctly.", tag)

# set grade and global result
feedback.set_grade(grade/(1.3))
if int(grade) == 130:
    feedback.set_global_result("success")
else:
    feedback.set_global_result("failed")