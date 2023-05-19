#!/bin/python3

# Emilie Deprez, 2022

from inginious import input, feedback
import json


def get_mapping(data, index):
    d = {}
    for key in data[index].keys():
        d[data[index][key]] = key
    return d


def get_index(random):
    return int(str(random)[2:4])

def checkFormat(string, mapping, flagQ234=False):
    d = {}
    existing_path = {"a":[["d", "e", "a"], ["f", "a"], ["g", "a"]], "b":[["h", "i", "b"], ["j", "b"]], "c":[["k", "c"], ["c"]], "l":[["m", "n", "l"], ["o", "l"], ["l"]], "100":[["100"]]}
    answer = string.lower().replace(" ", "").split(",")
    for p in answer:
        p_as = p.split("-")
        try:
            if len(p_as) != 2 or not p_as[0].startswith("p"): return (-1, "Wrong input format. Please provide your answer as a list <prefix>-<route> such as p1-AS2:AS1, p2-AS2, ...")
            AS = p_as[1].split(":")
            route = []
            for a in AS:
                if not a.startswith("as"): return (-1, "Wrong input format. Please provide your answer as a list <prefix>-<route> such as p1-AS2:AS1, p2-AS2, ...")
                if a[2:] != "100" and int(a[2:]) in mapping:
                    route.append(mapping[int(a[2:])])
                elif a[2:] != "100":
                    return (-1, f"Route for {p_as[0]} is not in the RIB")
                else:
                    route.append("100")
            
            if p_as[0][1:] == "100" and "100" not in d: d["100"] = route
            elif p_as[0][1:] == "100" or mapping[int(p_as[0][1:])] in d: return (-1, "There is always and only one preferred route for each prefix.")
            else : 
                d[mapping[int(p_as[0][1:])]] = route
                if flagQ234:
                    if (route[0] != "100"): return (-1, "Announced route should start with AS100")
                    if route[1:] not in existing_path[mapping[int(p_as[0][1:])]]: return (-1, f"Route for {p_as[0]} is not in the RIB")
                else:
                    if route not in existing_path[mapping[int(p_as[0][1:])]]: return (-1, f"Route for {p_as[0]} is not in the RIB")
        except:
            return (-1, "Wrong input format. Please provide your answer as a list <prefix>-<route> such as p1-AS2:AS1, p2-AS2, ...")
    return (0, d)

def checkQuestion(correct, fb, point, tag, mapping, data, index, flagQ234=False):
    answer = checkFormat(input.get_input(tag), mapping, flagQ234)
    error_comment = []
    if answer[0] == -1:
        feedback.set_problem_result("failed", tag)
        feedback.set_problem_feedback(answer[1], tag)
        return 0
    else : answer = answer[1]
    if flagQ234:
        for prefix in correct.keys():
            if correct[prefix] == [] and prefix in answer:
                error_comment.append(fb[prefix])
            elif correct[prefix] != [] and prefix not in answer:
                error_comment.append(fb[prefix])
            elif correct[prefix] != [] and correct[prefix] != answer[prefix]:
                error_comment.append(f"The route for prefix p{data[index][prefix]} is not the preferred")
    else:
        if len(answer) > len(correct):
            feedback.set_problem_result("failed", tag)
            feedback.set_problem_feedback("Too many route given", tag)
            return 0
        for prefix in correct.keys():
            if prefix not in answer:
                error_comment.append(f"Best route for prefix p{data[index][prefix]} not specified")
            elif correct[prefix] != answer[prefix]:
                error_comment.append(fb[prefix])
    if len(error_comment) == 0:
        feedback.set_problem_result("success", tag)
        feedback.set_problem_feedback("Correct", tag)
        return point
    else:
        feedback.set_problem_result("failed", tag)
        feedback.set_problem_feedback('; '.join(error_comment), tag)
        return 0    
 
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

tag_question = ["q1", "q2", "q3", "q4"]

grade += checkQuestion({"a" : ["d", "e", "a"], "b" : ["j", "b"], "c" : ["k", "c"], "l":["o", "l"]}, {"a" : "A route learned by a client is preferred over a route learned by a provider or through a shared-cost link", "b" : "When a route is learned by the same link type, the shortest one is preferred", "c": "A route learned by a shared-cost link is preferred over a route learned by a provider", "l":"A route learned by a client is preferred over a route learned by a provider and when a route is learned by the same link type, the shortest one is preferred."}, 25, "q1", mapping, data, index)

# advertise to a provider
grade += checkQuestion({"a" : ["100", "d", "e", "a"], "b" : [], "c" : [], "l":["100", "o", "l"], "100": ["100"]}, {"a" : "AS100 will annouce a prefix learned by a client to a provider", "b" : "AS100 will not announce a prefix learned by a provider to a provider", "c": "AS100 will not announce a prefix learned by a shared-cost to a provider", "100" : "AS100 will always advertise his prefix", "l":"AS100 will always advertise a prefixe learned by a client"}, 25, "q2", mapping, data, index, True)

# advertise to a client
grade += checkQuestion({"a" : ["100", "d", "e", "a"], "b" : ["100", "j", "b"], "c" : ["100", "k", "c"], "l":["100", "o", "l"], "100": ["100"]}, {"a" : "AS100 will annouce a prefix learned by a client to a client", "b" : "AS100 will announce a prefix learned by a provider to a client", "c": "AS100 will announce a prefix learned by a shared-cost to a client", "100" : "AS100 will always advertise his prefix", "l":"AS100 will always advertise a prefixe learned by a client"}, 25, "q3", mapping, data, index, True)

# advertise to a shared-cost
grade += checkQuestion({"a" : ["100", "d", "e", "a"], "b" : [], "c" : [], "l":["100", "o", "l"], "100": ["100"]}, {"a" : "AS100 will annouce a prefix learned by a client to a shared-cost", "b" : "AS100 will not announce a prefix learned by a provider to a shared-cost", "c": "AS100 will not announce a prefix learned by a shared-cost to a shared-cost", "100" : "AS100 will always advertise his prefix", "l":"AS100 will always advertise a prefixe learned by a client"}, 25, "q4", mapping, data, index, True)

feedback.set_grade(grade)
if int(grade) == 100:
    feedback.set_global_result("success")
else:
    feedback.set_global_result("failed")