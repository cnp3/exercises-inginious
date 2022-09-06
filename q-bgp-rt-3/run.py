#!/bin/python3

# Emilie Deprez, 2022

from inginious import input, feedback
import json

def toList(string):
    return string.replace(" ", "").split(",")


def pathToList(path):
    return path.split(":")

def get_mapping(data, index):
    d = {}
    for key in data[index].keys():
        d[data[index][key]] = key
    return d

def get_index(random):
    return int(str(random)[2:4])


def checkAnswer(tag_question, student, correct, feedback_list, point):
    """
        @pre : student and correct are list and student is already mapped
        @post : return the grade and set feedback
    """
    grade = 0
    if len(student) < len(correct):
        feedback.set_problem_result("failed", tag_question)
        feedback.set_problem_feedback("There are missing paths", tag_question)
        return 0
    if len(student) > len(correct):
        feedback.set_problem_result("failed", tag_question)
        feedback.set_problem_feedback("There are too many paths", tag_question)
        return 0
    for i in range(len(correct)):
        if student[i] != correct[i] and correct[i] =="0": # no route
            feedback.set_problem_result("failed", tag_question)
            feedback.set_problem_feedback("This AS does not have a route for the prefix", tag_question)
            return 0
        elif student[i] != correct[i] and student[i] =="0": # exist a route
            feedback.set_problem_result("failed", tag_question)
            feedback.set_problem_feedback("There is at least one route for the prefix", tag_question)
            return 0
        elif student[i] != correct[i] and pathToList(student[i])[-1] != pathToList(correct[i])[-1] : # end not with the right AS 
            feedback.set_problem_result("failed", tag_question)
            feedback.set_problem_feedback(feedback_list[i][0], tag_question)
            return 0
        elif student[i] != correct[i]:
            feedback.set_problem_result("failed", tag_question)
            feedback.set_problem_feedback(feedback_list[i][1], tag_question)
            return 0
    feedback.set_problem_result("success", tag_question)
    feedback.set_problem_feedback("Correct", tag_question)
    return point


def checkFormat(string, mapping):
    """
        @pre : string is the answer of the student and mapping the dictionnary (see mapRandom)
        @post : if the answer format is correct, return a list of routes where each AS is mapped
                else return an empty list
    """
    if string.strip() == "0": return ["0"]
    student = toList(string)
    mapped = []
    for elem in student:
        AS = pathToList(elem)
        asMapped = []
        for dom in AS:
            try:
                asMapped.append(mapping[dom])
            except:
                return []
        mapped.append(":".join(asMapped))
    return mapped


def check(student, mapping, correct, feedback_list, point, tag_question):
    """
        @post : check the answer format and return the grade for the question
    """
    answer = checkFormat(student, mapping)
    if answer == []:
        feedback.set_problem_result("failed", tag_question)
        feedback.set_problem_feedback("Wrong input format", tag_question)
        return 0
    else :
        return checkAnswer(tag_question, answer, correct, feedback_list, point)


# get all possible combinaisons
f = open("public/data.js", "r")
json_text = f.readlines()[0]
f.close()
json_text = json_text.replace("var data = ", "")
json_text = json_text.replace(";", "")
json = json.loads(json_text)

# set variables

grade = 0
index = get_index(input.get_input("@random")[0])

tag_question = ["q1", "q2", "q3", "q4", "q5"] # ID of each question
correct = {"q1": ["ASb:ASd", "ASc:ASf:ASb:ASd"], "q2": ["ASd"], "q3": ["ASf:ASb:ASd", "ASe:ASd"],"q4": ["ASd", "ASf:ASb:ASd"], "q5": ["ASb:ASd"]} # answer to each question (with AS used in mapping)
AS_ref = json[index]["ASa"]
# feedback for each question
feedback_list = {"q1": [[f"Your best path does not end to {AS_ref}", f"Your best path is incorrect"], [f"Your second path does not end to {AS_ref}", f"Your second path is incorrect"]], "q2": [[f"Your best path does not end to {AS_ref}", f"Your best path is incorrect"]], "q3": [[f"Your best path does not end to {AS_ref}", f"Your best path is incorrect"], [f"Your second path does not end to {AS_ref}", f"Your second path is incorrect"]],"q4": [[f"Your best path does not end to {AS_ref}", f"Your best path is incorrect"], [f"Your second path does not end to {AS_ref}", f"Your second path is incorrect"]], "q5": [[f"Your best path does not end to {AS_ref}", f"Your best path is incorrect"]] } 
mapping = get_mapping(json, index)

# check each question
for tag in tag_question:
    answer = input.get_input(tag)
    grade += check(answer, mapping, correct[tag], feedback_list[tag], 100/len(correct), tag)

# set grade and global result
feedback.set_grade(grade)
if int(grade) == 100:
    feedback.set_global_result("success")
else:
    feedback.set_global_result("failed")