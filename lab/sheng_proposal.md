Dr. Sheng — I'm Sean Patten, PhD student at Fordham in computer science. I attended your talk on Complex Vector Token Representation and I want to propose an experiment that connects your Wave Network to my research on quantum-classical model fusion. I think there's a convergence point between your work and mine that could produce a publishable result and answer your scaling question.

Let me lay out the connection.

---

YOUR WORK AS I UNDERSTAND IT

You showed that current self-attention has two problems: it's dominated by high-value tokens that attract disproportionate attention regardless of semantic relevance, and it's unstable under lexical perturbations — synonyms and typos produce different outputs when they shouldn't.

Your solution is CVTR — Complex Vector Token Representation. Each token becomes a complex vector in polar form, G times e to the i alpha. The magnitude G is the L2 norm across all tokens in a given dimension, capturing global semantics — what the whole input is about. The phase alpha is per-token, encoding how each token relates to that global meaning. Basic trig functions — sine, cosine, arctan2 — handle the polar-to-cartesian conversions. You showed that tokens, not whole words or characters, are the right granularity, and they map roughly to morpheme length.

Instead of softmax attention, you use two wave-physics operations. Wave interference — adding complex vectors — gives constructive or destructive combination based on phase alignment. Wave modulation — multiplying them — gives Z times Z prime equals G G prime times e to the i times alpha plus alpha prime. Multiply magnitudes, add phases.

The Wave Network achieves 91 percent on AG News with 2.4 million parameters versus BERT's 100 million. 77 percent less memory, 85 percent less training time. That's 97 percent of BERT's accuracy with 2.4 percent of the parameters.

The open questions from your talk: does it scale? Can anyone reproduce it? Does it generalize beyond classification to generation, question answering, time series?

---

MY WORK

I lead the Quantum Fusion project at Fordham under Dr. Hsu's supervision. We apply Combinatorial Fusion Analysis — CFA — to combine diverse models for stock movement prediction on time series data.

We have 36 quantum circuit models and 9 classical models, all benchmarked on 5 stock market datasets. The quantum models are implemented as C-backend statevector simulations. Each quantum circuit encodes input data via rotation gates — RY, RX, RZ — which are literally cosine and sine of theta over 2 applied to complex amplitudes. The circuits entangle qubits via CNOT gates, creating interference between complex amplitudes. Then they apply learned rotation parameters and measure.

Here's what I noticed reading your paper after your talk: my quantum circuits are doing the same mathematical operations as your Wave Network. Both use parameterized trig functions on complex amplitudes with interference. The rotation gates RY of theta equals cosine theta over 2 comma negative sine theta over 2 semicolon sine theta over 2 comma cosine theta over 2 — are polar-to-cartesian conversions of learned angles applied to complex state. CNOT creates wave interference. Measurement collapses the interference pattern to a prediction. Same primitives, different framing.

The difference is cost. My quantum circuits simulate 2 to the n complex amplitudes through the full statevector. For 8 qubits that's 256 complex numbers. Your CVTR operates in O of n times d instead of O of 2 to the n. Same trig, no quantum overhead.

---

THE NEUROSCIENCE CONNECTION

I believe there's a reason both approaches converge on the same primitives, and it has to do with how the brain actually processes information.

The cochlea literally does what your magnitude vector does. The basilar membrane decomposes sound into frequency components via physical resonance. It computes something like a Fourier transform in hardware. Your magnitude vector G sub k equals L2 norm across all tokens in dimension k — that's computing the energy in each representational frequency band across the whole input. The cochlea does this across the whole audio signal. Same operation, different substrate.

Gamma oscillations in the cortex — local processing — are amplitude-modulated by theta oscillations — global context. This is phase-amplitude coupling. The brain literally multiplies a local signal by a global carrier wave. Your wave modulation Z times Z prime equals G G prime times e to the i times alpha plus alpha prime is the same operation: multiply magnitudes, add phases.

All text decomposes to phonetic representation. All information can be carried in an audio stream alone to meaning. The brain's fundamental processing isn't fundamentally different from auditory signal processing — audio is the primitive. And humans exhibit robust probabilistic selection, which is exactly what wave interference patterns produce: constructive and destructive combination weighted by phase alignment.

Your work showing that wave representations achieve near-SOTA with extreme efficiency is consistent with this: if the brain evolved to process information via wave transforms because they're the most efficient basis, then artificial systems using the same basis should also be maximally efficient. Your parameter efficiency — 2.4 percent of BERT for 97 percent accuracy — is evidence of this.

Tokens mapping to morpheme length is also consistent. Morphemes are the minimal meaningful phonetic units. If the processing is fundamentally auditory-wave-like, the right granularity for decomposition should be the minimal unit of the auditory meaning stream — which is the morpheme.

---

THE EXPERIMENT I PROPOSE

I want to add a Wave Network model to my quantum fusion pipeline and test it against all 45 existing models using CFA.

Specifically:

1. Implement your CVTR as a new model in my timeseries/models directory. Take time series input — sequence by features. Represent each timestep as a complex vector using your magnitude-phase decomposition. Apply wave interference or modulation across timesteps. Sigmoid output for binary stock movement prediction.

2. Benchmark it against all 36 quantum circuits and 9 classical models on 5 stock market datasets using the same train-validation-test splits and evaluation protocol.

3. Apply CFA to the full ensemble including the wave model. CFA measures rank-score diversity between model pairs — models that are both accurate and diverse combine well. The question is whether the wave model provides a unique diversity profile compared to quantum circuits, given that they share trig primitives but differ in computational structure.

4. Compare the wave model's accuracy-per-parameter and accuracy-per-second against quantum circuits directly. If a wave model with O of n times d complexity performs comparably to quantum circuits with O of 2 to the n complexity on the same task, that's evidence that the trig-phase structure is doing the work, not the quantum entanglement. That's a publishable finding.

5. Test scaling: start with the classification-equivalent time series task, then extend to multi-step prediction and longer sequences. This directly addresses the open scaling question from your talk in a domain — finance — where both efficiency and accuracy have immediate practical value.

---

WHAT THIS COULD PRODUCE

If the wave model performs comparably to quantum circuits: we've shown that CVTR is a classical shortcut to quantum-circuit-level expressiveness, and CFA can identify when and where each approach dominates. That's a paper on efficient alternatives to quantum ML.

If the wave model provides unique CFA diversity: adding it to quantum-classical ensembles improves the fusion result, demonstrating practical value of CVTR beyond standalone classification. That's a paper on heterogeneous model fusion.

If both: we have evidence that parameterized trig on complex amplitudes is a fundamental computational primitive that works across architectures — quantum circuits, wave networks, and plausibly biological neural circuits — and CFA is the framework for combining them. That connects your representation theory to my fusion theory to the neuroscience.

I have the infrastructure, the datasets, the 45-model benchmark, and the CFA pipeline. Your CVTR is the missing model class. I can have initial results within weeks.

Would you be interested in collaborating on this?

---

Sean Patten
PhD Student, Computer Science, Fordham University
Quantum Fusion Project Lead
seanpattencode on GitHub
