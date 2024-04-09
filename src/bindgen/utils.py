def to_snake_case(identifier):
    snake_identifier = ""
    first = True
    last_had_underscore = False

    for char in identifier:
        if char.isupper():
            if not first and not last_had_underscore:
                snake_identifier += "_"
                last_had_underscore = True

            snake_identifier += char.lower()
        else:
            snake_identifier += char
            last_had_underscore = False

        first = False

    return snake_identifier


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