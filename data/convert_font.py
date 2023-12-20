from PIL import Image

def extract_pixel_data(image):
    """Extract pixel data from an indexed PNG image."""
    return list(image.getdata())

def convert_to_hex(packed_data):
    """Convert packed data (8 bits) to hex in format '0xnn'."""
    return f"0x{format(packed_data & 0xFF, '02X')},"

def write_to_file(hex_values, width):
    """Write hex values to a text file with newlines based on width."""
    with open("pico_font.h", 'w') as file:
        file.write(f"static const uint8_t font_map[] = {{\n")
        for index, hex_val in enumerate(hex_values, start=1):
            file.write(hex_val)
            if index % (width // 8) == 0 and index != len(hex_values):
                file.write('\n')
        file.write("\n};")

def main(input_png_path):
    # Open the PNG image
    with Image.open(input_png_path) as img:
        # Check if the image is an indexed PNG
        if img.mode != 'P':
            print("The image is not an indexed PNG.")
            return
        
        width = img.width
        
        # Get the pixel data (indexed values)
        pixel_data = extract_pixel_data(img)
        
        # Convert pixel data to binary format (1 bit per pixel)
        binary_data = []
        for pixel in pixel_data:
            binary_data.append(0 if pixel == 0 else 1)
        
        # Convert binary data (1-bit pixels) to hex (8-bit bytes)
        hex_values = []
        for i in range(0, len(binary_data), 8):
            # Pack 8 bits into a single byte
            byte_data = 0
            for j in range(8):
                byte_data |= (binary_data[i + j] << (7 - j))
            hex_values.append(convert_to_hex(byte_data))
        
        # Write to the specified output file path
        write_to_file(hex_values, width)
        
        print(f"Hex values written to pico_font.h.")

if __name__ == "__main__":
    main("pico8_font.png")
