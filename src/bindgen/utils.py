from enum import Enum

def to_snake_case(identifier):
    result = ""

    for i in range(len(identifier)):
        char = identifier[i]
        prev_char = identifier[i - 1] if i != 0 else None
        next_char = identifier[i + 1] if i != len(identifier) - 1 else None
        
        if char.islower():
            result += char
        elif char.isupper():
            if prev_char and not prev_char.isupper() and prev_char != "_":
                result += "_"
            if prev_char and next_char and prev_char.isupper() and next_char.islower():
                result += "_"

            result += char.lower()
        elif char.isdigit():
            if prev_char and not prev_char.isdigit():
                result += "_"
            
            result += char
        elif char == "_":
            result += char

    return result


def to_pascal_case(identifier):
    pascal_identifier = ""
    next_upper = True

    for char in identifier:
        if char == "_":
            next_upper = True
        elif next_upper:
            pascal_identifier += char.upper()
            next_upper = False
        else:
            pascal_identifier += char.lower()
    
    return pascal_identifier


def common_prefix_len(strings):
    if not strings:
        return 0

    for i in range(min([len(s) for s in strings])):
        c = strings[0][i]
        for s in strings:
            if s[i] != c:
                return i
    return i