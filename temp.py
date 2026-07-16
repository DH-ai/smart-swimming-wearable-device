import random
import math

def generate_swimming_data(filename="swimming_data.txt", num_strokes=5, glitch_chance=0.03):
    current_time = 0.0
    dt = 0.05  # Time step: 50 milliseconds per sensor reading
    
    with open(filename, "w") as file:
        for stroke in range(1, num_strokes + 1):
            
            air_readings = 120 
            for _ in range(air_readings):
                velocity = random.uniform(3.9, 4.7)
                
                if random.random() < glitch_chance:
                    velocity = random.uniform(12.0, 18.0)
                    
                file.write(f"{current_time:.2f} {velocity:.2f} 0\n")
                current_time += dt
                
            
            water_readings = 200 
            for _ in range(water_readings):
                velocity = random.uniform(1.3, 1.9)
                
                if random.random() < glitch_chance:
                    velocity = random.uniform(0.0, 0.2)
                    
                file.write(f"{current_time:.2f} {velocity:.2f} 1\n")
                current_time += dt

    print(f"[SUCCESS] Generated '{filename}' with {num_strokes} complete stroke cycles.")
    print(f"Total data points created: {num_strokes * 32}")

if __name__ == "__main__":
    generate_swimming_data("swimming_data.txt", num_strokes=8)
