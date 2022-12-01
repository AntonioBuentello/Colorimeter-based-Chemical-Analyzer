# CSE3442
 Colorimeter-based Chemical Analyzer built for TM4C123GH6PM

 The University of Texas at Arlington | Junior Year | CSE 3442 | Embedded I
 This Project was programed in C for bare metal operation on the TI Tiva Development Board.
 This project was developed and created in CCStudio.

# Overview
The students constructed a device to handle chemical samples is test tubes spun around by a
turret. The chemical samples’ pH levels could then be analyzed using a colorimetry approach.
Specifically, the color of each sample tube will differ based on their pH level, thus allowing the
students to shine a RGB through the samples and finding the amount of refraction experienced
with a phototransistor. The refraction values were then evaluated to find a formula to estimate a
sample’s pH.

# UART Commands
## home
Moves turret to reference position and stores home position

## turret TUBENUM
Rotates the turret to position 1, 2, 3, 4, 5, or R (reference)

## calibrate
Rotates the turret to reference position and calibrates the light path

## measure TUBENUM raw
Rotates the turret to position 1, 2, 3, 4, 5, or R (reference) and displays the 
raw RGB data values from the A/D converter

## measure TUBE pH | Cl
Rotates the turret to position 1, 2, 3, 4, 5, or R (reference) and displays the 
pH or chlorine values
