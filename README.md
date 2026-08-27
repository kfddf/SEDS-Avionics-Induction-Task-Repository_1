# SEDS-Avionics-Induction-Task-Repository_1
This repository contains the codes used in both the 1st & 2nd SEDS Avionics Tasks.

Athena's Intern — SEDS BPHC Avionics Task
Hey there! This is my submission repo for the SEDS BPHC Avionics Round 1 Induction Task. The task is themed around helping Odysseus and his crew navigate safely back to Ithaca using code and embedded systems.

Honestly, tackling this was a huge learning curve for me—especially getting matplotlib to play nice with continuous animations and working through non-blocking timing logic on the Arduino without tearing my hair out. Here is how I figured everything out and built both tasks from scratch!

Task 1: Finding the Sea Floor (Python Depth Visualizer)
For the first task, I needed to take raw depth data from a text file, clean out any bad readings, smooth out the noisy data, and animate a live depth graph.

My Approach & Step-by-Step Logic
Reading the Data: I created load_raw_readings() just to read the file safely using Python's standard with open(...) structure so I wouldn't leave open file handles laying around.

Cleaning Bad Sensor Readings: Sonar data gets messy, so clean_readings() handles the cleanup:

It ignores empty lines so the script doesn't crash on blank spaces.

It uses a try/except block to convert each line to a float. If a line is pure gibberish, it skips it and prints a warning message showing the line number.

Sensors sometimes output extreme glitch values (like negative numbers or crazy spikes like 99999). I set boundary checks (min_depth=0.0 and max_depth=100.0) to drop any reading that doesn't make sense physically.

Smoothing Out Noise: Real sensor data bounces around too much. To fix this, I wrote moving_average(). It slides a small window across the numbers, averaging each point with its immediate neighbors to level out random spikes while keeping the actual sea floor shape intact.

Animating the Graph: Setting up animate_depth_chart() was tricky!

I flipped the vertical axis using ax.invert_yaxis() so deeper ocean depth goes downward, which visually makes way more sense.

I used FuncAnimation with blit=True so it continuously updates the line without re-drawing the whole figure every single frame.

Troubleshooting Note: On my machine, the visualizer window kept freezing, so I had to force the rendering backend to matplotlib.use("TkAgg") at the very top of Task 1's code to keep the pop-up window open smoothly.

Task 2: Keeping Watch Over Odysseus (Tinkercad State Machine)
For the second task, I had to design a 5-state safety system using an Arduino Uno, an Ultrasonic Sensor, a Photoresistor (Light Sensor), a Pushbutton, an LCD Screen, an LED, and a Buzzer.

How I Handled the 5 States
OPEN SEA (Default State): This is the normal starting condition. The LCD displays "OPEN SEA" and everything runs quietly.

ANCHOR DROPPED: Pressing the pushbutton toggles the anchor down. When the anchor is down, the ship is 100% safe from all incoming dangers. Pressing it again raises the anchor back up.

STORM: Triggered when light levels drop below the threshold (dark environment). While in a storm, the LED blinks as a warning visual.

CHARYBDIS: Triggered when the ultrasonic sensor detects an obstacle closer than 100 cm. The buzzer sounds to warn the ship.

WRECKED: If the ship spends 5 consecutive seconds in STORM or CHARYBDIS without the user dropping the anchor, the ship wrecks! Once wrecked, it latches into this fail state until the simulation restarts.

Challenges I Ran Into & Solved
Avoiding delay(): My biggest hurdle was tracking the 5-second countdown timer. Using Python-style pauses or delay() in Arduino completely freezes the code, meaning button presses get missed and the LCD lags. I switched to using millis() to track timestamps in the background while keeping the main loop spinning smoothly.

I wrote this README file, however using few AI tools such as Gemini, and Claude to make my work more presentable and sound a little technical as well as professional at scale, for cutting down unnecessary and redundant use of vocabulary, and paraphrasing sentences and etc.

Priority Handling: If both a storm and Charybdis happen at once, my logic locks into whichever hazard hit first so the 5-second wreck timer doesn't get messed up midway through.terpreter path until a clean, isolated conda environment (plotenv) was set up and used consistently.
