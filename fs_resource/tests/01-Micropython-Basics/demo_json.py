import json

json_str = '''{
    "name": "kendryte",
    "babies": [
        {
            "name": "canmv",
            "birthday": 220913,
            "sex": "unstable"
        }
    ]
}'''

obj = json.loads(json_str)
print(obj["name"])
print(obj["babies"])
