import json

# Read the characters from the p8scii.json file
with open('p8scii.json', 'r') as file:
    characters = json.load(file)

p8symbols = []

# Filter and format characters
for index, char in enumerate(characters):
    encoded_length = len(char.encode('utf-8'))
    
    # Skip standard ASCII characters
    if encoded_length == 1 and 0x20 <= ord(char) < 0x7F:
        continue
    else:  # Multi-byte characters
        encoded_char = [hex(b)[2:].zfill(2) for b in char.encode('utf-8')]
        p8symbols.append({'encoding': encoded_char, 'length': encoded_length, 'index': index})

# Write to the p8_symbols.h file
with open('p8_symbols.h', 'w') as file:
    # Write C code for p8_symbol_t
    file.write("typedef struct\n{\n")
    file.write("    uint8_t encoding[7];\n")
    file.write("    uint8_t length;\n")
    file.write("    uint8_t index;\n")
    file.write("} p8_symbol_t;\n\n")
    
    file.write("static const p8_symbol_t p8_symbols[] = {\n")
    for symbol in p8symbols:
        file.write("    {")
        file.write("{" + ", ".join([f"0x{byte}" for byte in symbol['encoding']]) + "}, ")
        file.write(str(symbol['length']) + ", " + str(symbol['index']) + " },\n")
    file.write("};\n")
