# Derivation of formulas to convert words per minute to dot length and reverse
@isaac-silversat

2023-07-11

**This is a work in progress. The conversion factor does not make sense because it causes dot times to increase with code speed.**

This is the scratchpad used to develop formulas to convert words per minute to
dot length and reverse.

# Dot Length to Words per Minute

A unit conversion is applied to derive a conversion factor. First, words per minute is coverted to words per second.

$\frac {n \text{ words}} {\text{minute}} \times \frac {1 \text{ minute}} {60 \text{ seconds}} = \frac {n \text{ words}} {60 \text{ seconds}}$ where $n$ is the Morse code speed.

According to Bern, D. (n.d.; personal communication), the standard word for
Morse code speed is PARIS (**.--. .- .-. .. ...**) which is 10 dots + 
(4 spaces * 3 dots/space) + (4 dashes * 3 dots/dash) = 34 dots long. 
Therefore,

$\frac {n \text{ words}} {\text{minute}}  = \frac {n \text{ words}} {60 \text{ seconds}} \times \frac{34 \text{ dots}}{\text{word}} = \frac {17n \text{ dots}} {34 \text{ seconds}}$

Therefore,

$\frac{1} {n \frac {\text{words}} {\text{minute}}} = \frac  {30 \text{ seconds}} {17n \text{ dots}}$

Converting to milliseconds and applying a 50% duty cycle,

$\frac {\text{milliseconds on}} {\text{dot}} = \frac {\text{seconds}} {\text{dot}} \times \frac {1000 \text{ milliseconds}} {\text{second}} \times 0.5 = 500 \times \frac {\text{seconds}} {\text{dot}}$