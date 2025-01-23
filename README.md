Kuramoto Model Simulation with Pendulum Visualization.
Description.
This program implements a simulation of the Kuramoto model for synchronizing phase oscillators. The visualization is designed for the Arduboy platform and includes:

 - A rotating point on a circle, linked to the oscillators' phases.
 - A pendulum whose motion is synchronized with the rotating point.
 - A sinusoidal graph with point-based rendering synchronized with the model.
 - Real-time parameter adjustments.
 - A sound effect synchronized with the pendulum's motion.

Features:
1. Kuramoto Model Simulation:

- Visual representation of phase synchronization between oscillators.
- Adjustable number of oscillators N, coupling coefficient K, and time step DT.

Interactive Controls:

 - B Button: Adjust the time step DT.
 - UP and DOWN (while holding B): Modify the coupling coefficient K.
 - UP and DOWN (without B): Change the number of oscillators N.
 - LEFT and RIGHT: Adjust oscillator frequencies.
 - A Button: Toggle sound on/off.

Visualization:

 - Circular trajectory for oscillator phases.
 - Pendulum synchronized with the oscillators' phases.
 - Point-based sinusoidal graph for additional visual feedback.

Sound Effect:

 - A sound is emitted at every full cycle of the pendulum, creating a metronome-like effect.
 - How to Use Upload the program to an Arduboy device.

Use the buttons to adjust parameters:

 - Modify K, N, and DT to observe changes in synchronization.
 - Observe the sinusoidal graph, pendulum, and sound for insights into the model's dynamics.
 - Watch in real-time as the pendulum and points synchronize or desynchronize with parameter adjustments.

main.ino: The main program code.
README.md: Project description.

Future Improvements:

 - Add support for more complex oscillator coupling models.
 - Implement a graph to display the synchronization level.
 - Introduce additional visual and sound effects to enhance the demonstration.

Requirements:
 - Arduboy device.
 - Arduboy2 Library (https://github.com/MLXXXp/Arduboy2).
 - Arduino IDE for compilation.