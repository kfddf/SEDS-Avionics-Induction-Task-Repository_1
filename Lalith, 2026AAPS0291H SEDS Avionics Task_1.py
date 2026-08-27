import matplotlib                                  # Import the matplotlib library so we can configure it before plotting
matplotlib.use("TkAgg")                             # Force the Tk-based rendering backend so the plot window opens and stays open on Windows
import matplotlib.pyplot as plt                     # Import the plotting interface, nicknamed as "plt"
from matplotlib.animation import FuncAnimation      # Import FuncAnimation, the class that lets us redraw the chart on a timer for animation

# -------------------------------------------------------------
# SETTINGS 
# -------------------------------------------------------------
DATA_FILE = "Sample_Text.txt"   # Name of the file containing raw depth readings; 
MIN_VALID_DEPTH = 0             # Any reading below this (e.g. negative sentinel values like -999) is treated as sensor failure
MAX_VALID_DEPTH = 100           # Any reading above this (e.g. 99999) is treated as an impossible reading
WINDOW_SIZE = 3                 # Number of consecutive points averaged together to smooth out noise
FRAME_INTERVAL_MS = 1000        # Milliseconds between animation frames; 1000 = one new point revealed per second, matching the 1 reading/sec assumption


# -------------------------------------------------------------
# STEP 1: Read the raw data out of the file
# -------------------------------------------------------------
def load_raw_readings(filename):                   # Define a function that takes a filename and returns its raw lines
    with open(filename, "r") as f:                  # Open the file in read mode; "with" makes sure it auto-closes afterward
        lines = f.readlines()                       # Read every line as a list of strings, one entry per line
    return lines                                    # Hand the raw lines back to the caller


# -------------------------------------------------------------
# STEP 2: Clean the data 
# -------------------------------------------------------------
def clean_readings(raw_lines):                      # Define a function that filters bad readings out of the lines
    cleaned = []                                    # Start an empty list to collect only the valid readings
    for i, line in enumerate(raw_lines, start=1):   # Loop through every line, tracking a 1-based line number for clear error messages
        line = line.strip()                         # Remove trailing whitespace and newline characters
        if not line:                                # Check if the line is empty after stripping
            continue                                # Skip blank lines entirely rather than treating them as bad data
        try:                                        # Attempt to convert text to a number
            value = float(line)                     # Convert the string to a floating-point number
        except ValueError:                          # Catch the case where the text isn't a valid number at all
            print(f"Line {i}: skipping non-numeric value '{line}'")  # Report exactly which line was rejected and why
            continue                                # Move on to the next line without adding this one
        if value < MIN_VALID_DEPTH or value > MAX_VALID_DEPTH:  # Check whether the number falls outside depth range
            print(f"Line {i}: skipping out-of-range value {value}")  # Report the specific out-of-range value that was dropped
            continue                                # Skip this reading; don't let false sensor spike into the dataset
        cleaned.append(value)                       # If it passed both checks, keep this reading
    return cleaned                                  # Return the fully filtered list of trustworthy readings


# -------------------------------------------------------------
# STEP 3: Smooth the data with a moving average
# -------------------------------------------------------------
def moving_average(values, window_size=WINDOW_SIZE):  # Define a function that smooths a list of numbers using a sliding window
    smoothed = []                                    # Start an empty list to collect the smoothed output values
    for i in range(len(values)):                     # Loop over every index in the input list
        start = max(0, i - window_size + 1)          # Compute the earliest index to include, never going below 0 
        end = i + 1                                  # The window always ends at (and includes) the current index
        window = values[start:end]                   # Slice out just the values in the current window
        average = sum(window) / len(window)          # Compute the mean of the values inside this window
        smoothed.append(average)                     # Store the smoothed value for this position
    return smoothed                                  # Return the full list of smoothed values, same length as the input


# -------------------------------------------------------------
# STEP 4: Animate the depth-vs-time chart
# -------------------------------------------------------------
def animate_depth_chart(depths, interval_ms=FRAME_INTERVAL_MS):  # Define the function that builds and plays the animation
    times = list(range(len(depths)))                # Build a simple 0,1,2,3... time axis, one tick per reading (1 reading/sec assumption)
    fig, ax = plt.subplots()                        # Create a new figure (the window) and a single set of axes to draw on
    ax.set_xlim(0, len(times))                      # Fix the x-axis range up front so the chart doesn't resize as points are added
    ax.set_ylim(min(depths) - 1, max(depths) + 1)   # Fix the y-axis range with 1m of padding above/below so the line isn't glued to the edges
    ax.set_xlabel("Time (seconds)")                 # Label the x-axis 
    ax.set_ylabel("Depth (meters)")                 # Label the y-axis
    ax.set_title("Sea Floor Depth Over Time")       # Give the whole chart a title
    ax.invert_yaxis()                               # Flip the y-axis so larger depth values are drawn lower

    line, = ax.plot([], [], color="steelblue", linewidth=2, marker="o", markersize=3)
    # ^ Create an initially empty line object to be updated frame-by-frame; the trailing comma unpacks the single-item list ax.plot() returns

    def update(frame):                              # Define the function FuncAnimation will call once per frame, receiving the frame number
        current_times = times[: frame + 1]          # Slice the time axis up to and including the current frame (reveals data progressively)
        current_depths = depths[: frame + 1]        # Slice the depth data the same way, so both arrays stay the same length
        line.set_data(current_times, current_depths)  # Update the existing line object's data instead of redrawing the whole chart from scratch
        return (line,)                              # Return the changed artists as a tuple, which FuncAnimation requires when using blit=True

    ani = FuncAnimation(                            # Create the animation object that drives the whole thing
        fig,                                        # Which figure/window to animate
        update,                                     # The function to call on every frame
        frames=len(times),                          # Total number of frames to play, one per data point
        interval=interval_ms,                       # Delay between frames in milliseconds
        blit=True,                                  # Optimize performance: only redraw parts of the plot that actually change
        repeat=False,                               # Play through once instead of looping forever
    )

    plt.tight_layout()                              # Automatically adjust spacing so labels/titles don't cut off
    plt.show()                                      # Actually open the window and start playing the animation; nothing appears before this line
    return ani                                      # Return the animation object so it isn't garbage-collected (deleted) mid-playback


# -------------------------------------------------------------
# MAIN PROGRAM
# -------------------------------------------------------------
if __name__ == "__main__":                          # Only run the following block when this file is executed directly, not when imported
    try:                                             # Attempt to load the data file, since it might be missing or misnamed
        raw_lines = load_raw_readings(DATA_FILE)    # Call Step 1 to pull the raw text lines out of the file
    except FileNotFoundError:                       # Catch the specific error Python raises when the file doesn't exist
        print(f"ERROR: Could not find '{DATA_FILE}'.")           # Tell the user what went wrong
        print("Make sure Sample_Text.txt is in the SAME FOLDER as this script.")  # Give them the exact fix
        raise SystemExit(1)                         # Stop the program immediately with a non-zero exit code, since nothing else can proceed

    print(f"Read {len(raw_lines)} raw lines from file.")  # Confirm how much raw data was actually found

    clean_depths = clean_readings(raw_lines)        # Call Step 2 to filter out irregular readings
    print(f"Kept {len(clean_depths)} valid readings after cleaning.")  # Report how many readings survived cleaning

    if len(clean_depths) == 0:                      # Guard against the edge case where every single reading was rejected
        print("ERROR: No valid readings left after cleaning. Check DATA_FILE and thresholds.")  # Explain the likely cause
        raise SystemExit(1)                         # Stop the program since there's nothing left to plot

    smoothed_depths = moving_average(clean_depths)  # Call Step 3 to reduce noise via the moving average
    print("Smoothed depths:", [round(d, 2) for d in smoothed_depths])  # Print the smoothed values, rounded for readable output

    animation_obj = animate_depth_chart(smoothed_depths)  # Call Step 4 to draw and play the animated depth-vs-time chart