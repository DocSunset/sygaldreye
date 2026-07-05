# hello-cosine golden audio — properties, not bytes

The rung-2 and rung-5 gates assert PROPERTIES of the rendered output
(determinism class: approximate — hash equality is never the test):

1. Dominant spectral peak at 220 Hz, plus-or-minus 1 Hz, at least 40 dB above
   the next non-harmonic component.
2. Amplitude envelope periodic at 0.5 Hz, plus-or-minus 5%, modulation depth
   per the lfo/vca defaults.
3. No sample outside [-1, 1]; no NaN/Inf anywhere in the buffer.
4. Phase continuity across a mid-render swap (EXE-5.1): no discontinuity
   exceeding one block boundary when freq changes 220 -> 330.

BLESSING (ch. 17): once rung 2 first sounds, Travis listens and blesses one
rendered take by committing it as a capture; thereafter regression compares
against these properties AND spectral distance to the blessed take within
the tolerance comparator. The blessing is testimony — signed, dated,
supersedable.
