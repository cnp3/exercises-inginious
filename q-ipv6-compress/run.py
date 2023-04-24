#!/bin/python3

# Emilie Deprez, 2022

from inginious import input, feedback
import ipaddress

def get_index(random):
    return int(str(random)[2:4])

def check(answer, correct, tag_question):
    try:
        addr_answer = ipaddress.IPv6Address(answer.strip())
    except ipaddress.AddressValueError as message:
        feedback.set_problem_result("failed", tag_question)
        feedback.set_problem_feedback("It's not a valid IPv6 address : " + str(message), tag_question)
        return 0
    addr_correct = ipaddress.IPv6Address(correct)
    if addr_answer != addr_correct:
        feedback.set_problem_result("failed", tag_question)
        feedback.set_problem_feedback("The IPv6 address is not the same as the one given", tag_question)
        return 0
    
    if str(addr_correct.exploded) == str(answer.strip()):
        feedback.set_problem_result("failed", tag_question)
        feedback.set_problem_feedback("The IPv6 address is not compressed", tag_question)
        return 0
    if str(addr_correct.compressed) != str(answer.strip()):
        feedback.set_problem_result("success", tag_question)
        feedback.set_problem_feedback(f"Correct but there is a more compressed form ``{str(addr_correct.compressed)}``", tag_question)
        return 12.5
    feedback.set_problem_result("success", tag_question)
    feedback.set_problem_feedback("Correct", tag_question)
    return 12.5


# get all possible combinaisons
f = open("public/data.js", "r")
json_text = f.readlines()[0]
f.close()
json_text = json_text.replace("var data = ", "")
json_text = json_text.replace(";", "")
json = json.loads(json_text)

grade = 0
index = get_index(input.get_input("@random")[0])

for tag in ["q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8"]:
    answer = input.get_input(tag)
    grade += check(answer, json[index][tag], tag)


feedback.set_grade(grade)
if int(grade) == 100:
    feedback.set_global_result("success")
else:
    feedback.set_global_result("failed")