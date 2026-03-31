# Wave-Quantum Convergence: Connecting CVTR, Quantum Circuit Models, and Biological Wave Processing

## Sean Patten — Fordham University, PhD Computer Science

---

## PART 1: THE LANDSCAPE

### 1.1 Sheng's Wave Network and CVTR

Victor S. Sheng (Texas Tech University) and Xin Zhang published the Wave Network in November 2024 (arXiv:2411.02674). The paper identifies two failure modes in transformer self-attention:

**Attention sink dominance.** Softmax attention concentrates on a few high-value tokens — often punctuation, [CLS], or the first token — regardless of semantic relevance. These tokens attract attention because the softmax exponential amplifies small score advantages. The result is that most tokens are effectively ignored even when they carry meaning.

**Lexical perturbation instability.** Replace "big" with "large" or introduce a typo and the output changes. Dot-product attention is sensitive to exact embedding vectors. Small changes in tokens cascade through layers unpredictably because attention weights shift globally when any single token's embedding shifts.

Sheng's solution: Complex Vector Token Representation (CVTR). Each token is represented as a complex number in polar form:

```
Z_j = G * e^(i * α_j)
```

Two components:

- **Magnitude G** (global semantics): For each dimension k across n tokens, G_k = sqrt(w_{1,k}^2 + w_{2,k}^2 + ... + w_{n,k}^2). This is the L2 norm — the total "energy" in that representational dimension across all tokens. Every token shares the same magnitude vector. It captures what the entire input is about.

- **Phase α** (local semantics): Per-token, α_k = arctan2(sqrt(1 - (w_{j,k}/G_k)^2), w_{j,k}/G_k). This encodes each token's individual relationship to the global meaning.

Two update operations replace self-attention:

**Wave interference** (addition of complex vectors):
```
Z_interference = Z + Z'
|Z_total|^2 = G^2 + G'^2 + 2*G*G'*cos(α_j - α'_j)
```
The interference term 2*G*G'*cos(α - α') determines constructive or destructive combination based on phase alignment. If two tokens' phases agree, they reinforce. If opposed, they cancel. No softmax. No attention sink. Every token contributes through physics, not competition.

**Wave modulation** (multiplication of complex vectors):
```
Z_modulation = Z * Z' = G*G' * e^(i*(α_j + α'_j))
```
Magnitudes multiply, phases add. This is amplitude modulation — a local signal modulated by a global carrier.

The implementation uses basic trig: sin, cos, arctan2 for polar-to-cartesian conversions. Attention calculation done in PyTorch. Tokens (not whole words or characters) are the right granularity — they map roughly to morpheme length.

**Results:** 91.66% accuracy on AG News text classification with 2.4M parameters vs BERT's 94.64% at 100M parameters. 77% less memory, 85% less training time. The Wave Network outperforms a single transformer layer by ~19 percentage points even starting from random embeddings.

**Limitations as of March 2026:**
- No code released (people asked on Hacker News, no response)
- Classification only — never tested on generation, QA, or time series
- Only the original authors have published on it (one follow-up paper on backpropagation dynamics, arXiv:2411.06989)
- No independent reproduction or citation
- Scaling behavior completely unknown

**Competing approaches that did scale:**
- FNet (Google, 2021): Fourier transform replaces attention, 92-97% of BERT, 80% faster
- SPECTRE (Feb 2025): FFT + spectral gating, 7x faster than FlashAttention-2 at 128k context
- Mamba-3 (ICLR 2026): Complex-valued state space models at 1.5B parameters
- Hyena (ICML 2023): Long convolutions via FFT, scaled to 1.3B parameters

### 1.2 My Quantum Fusion Work

The Quantum Fusion project at Fordham under Dr. Hsu applies Combinatorial Fusion Analysis (CFA) to ensemble diverse models for stock movement prediction.

**The pipeline (quantumfusion/timeseries/run.py):**
- 36 quantum circuit models + 9 classical models = 45 total
- 5 stock market datasets (StockNet, KDD17, CSI300, FTSE100, NDX100, NI225)
- Train/validation/test splits with standardized evaluation
- CFA fusion at the end: rank-score characterization, cognitive diversity measurement, greedy ensemble selection

**The quantum models (c_quantum.c):**
All implemented as C-backend statevector simulations compiled at runtime. 10 circuit architectures: QLSTM, tree, RyRz, reupload, IQP, QAOA, hardware, brickwall, star, multiaxis. Each follows the same pattern:

1. Initialize statevector |000...0⟩
2. Encode input data via rotation gates (RY, RX, RZ)
3. Entangle qubits via CNOT
4. Apply learned rotation parameters
5. Measure ⟨Z⟩ on qubit 0
6. Wrap in LSTM gating structure (input/forget/output gates each powered by a circuit)
7. Adam optimizer with numerical gradients

Training outputs per-sample predictions for CFA fusion downstream.

**CFA mechanics:**
- Normalize each model's predictions to [0,1]
- Compute Rank-Score Characteristic (RSC) curves
- Measure Cognitive Diversity (CD) between model pairs: CD = sqrt(sum((RSC_a - RSC_b)^2) / n)
- Combine via multiple methods: Average Score (ASC), Average Rank (ARC), Weighted by Diversity (WSCDS/WRCDS), Weighted by Performance (WSCP/WRCP)
- Greedy selection: start with best individual model, add models that improve ensemble accuracy on validation set

---

## PART 2: THE MATHEMATICAL CONVERGENCE

### 2.1 Quantum Circuits and CVTR Are the Same Math

A quantum rotation gate RY(θ) applied to a qubit in state [α, β]^T computes:

```
RY(θ) = [[cos(θ/2), -sin(θ/2)],
          [sin(θ/2),  cos(θ/2)]]
```

This is a rotation in complex amplitude space parameterized by data (θ = input feature) or learned weights. The qubit's state after rotation is a complex number whose magnitude and phase have been modified by the trig of the input.

RX and RZ similarly:
```
RX(θ): applies cos(θ/2) and -i*sin(θ/2) — phase rotation on imaginary axis
RZ(θ): applies e^(-iθ/2) and e^(iθ/2) — pure phase rotation
```

**CNOT creates wave interference.** When a control qubit's amplitude conditions the target qubit's state flip, the resulting statevector contains cross-terms between the two qubits' amplitudes. This is mathematically identical to wave interference — two complex amplitudes combining constructively or destructively based on their relative phase.

**Measurement collapses to a real prediction.** sv_z0() computes ⟨Z⟩ = Σ|α_k|^2 for k with bit0=0 minus k with bit0=1. This extracts a real value from the interference pattern — the same role as taking the real part or magnitude of the combined complex representation in CVTR.

**The QLSTM wrapper adds gating.** Each of the four LSTM gates (input, forget, cell candidate, output) runs a separate quantum circuit with different learned parameters. The gate outputs control information flow through the recurrent cell. This is structurally identical to using wave interference/modulation to gate information — the same function, wrapped in the same LSTM structure.

**The key difference is computational cost:**

| Operation | Quantum Circuit (statevector sim) | CVTR |
|---|---|---|
| State representation | 2^n complex amplitudes | n*d real numbers |
| Data encoding | Rotation gates O(2^n) | L2 norm + arctan2 O(n*d) |
| Interaction | CNOT O(2^n) | cos/sin of phase difference O(n*d) |
| Output | Expectation value O(2^n) | Sigmoid of combined magnitude O(d) |
| 8-qubit example | 256 complex numbers per step | 8*d real numbers per step |

Both use the same trig functions (sin, cos, arctan2) on the same mathematical objects (complex amplitudes with magnitude and phase). The quantum circuit traverses an exponentially large state space; CVTR gets the magnitude-phase decomposition directly.

### 2.2 What Quantum Entanglement Adds (and Might Not)

The quantum circuit's 2^n state space allows correlations between qubits that cannot be decomposed into individual qubit states. This is entanglement — it provides a representational capacity that grows exponentially with qubit count.

But for the stock prediction task, the question is: does this exponential capacity actually help, or does the data not have enough structure to exploit it?

If a CVTR model with O(n*d) complexity matches quantum circuits with O(2^n) complexity on the same data, the answer is clear: the trig-phase structure is doing the useful work, and entanglement is adding computation without proportional benefit. This would be a significant finding for quantum ML.

If quantum circuits consistently outperform CVTR, that's evidence that entanglement captures genuine high-order correlations in the data. Also significant — it would justify the computational cost.

CFA can resolve this because it measures not just accuracy but diversity. Even if CVTR and quantum circuits achieve similar accuracy, they might make different errors on different samples. CFA's cognitive diversity metric would detect this, and the fusion result would tell us whether combining both approaches beats either alone.

---

## PART 3: THE NEUROSCIENCE BRIDGE

### 3.1 The Cochlea as Magnitude Computation

The basilar membrane of the cochlea performs spectral decomposition of incoming sound. Different positions along the membrane resonate at different frequencies — high frequencies at the base, low at the apex. Each position computes, through physical resonance, the energy present in its frequency band across the entire incoming signal.

This is functionally identical to Sheng's magnitude vector. G_k = L2 norm of all token weights in dimension k = the total energy in representational dimension k across all tokens. The cochlea computes the total energy in frequency band k across the whole audio signal. Same operation: aggregate energy per channel across the full input.

### 3.2 Phase-Amplitude Coupling as Wave Modulation

In the cortex, gamma oscillations (30-100 Hz) carry local processing — the content of what's being processed at a given site. Theta oscillations (4-8 Hz) carry global context — what processing mode the brain is in, what's being attended to.

These are coupled: the amplitude of gamma is modulated by the phase of theta. This is phase-amplitude coupling (PAC). When theta is at its peak phase, gamma amplitude is high; when theta is at its trough, gamma is suppressed.

Mathematically, this is multiplication of a local signal by a global carrier wave:

```
Signal_combined = A_gamma(t) * cos(ω_theta * t + φ_theta)
```

Sheng's wave modulation:
```
Z_modulation = G*G' * e^(i*(α + α'))
```

Both operations: multiply magnitudes (local amplitude modulated by global amplitude), add phases (local timing referenced to global timing). The brain literally does wave modulation.

### 3.3 All Language is Audio First

All text decomposes to phonetic representation. Written language is a visual encoding of spoken language — the mapping is lossy but the direction is clear. All information can be carried in an audio stream alone to reach meaning. Humans processed audio for hundreds of thousands of years before writing existed.

The brain's language processing isn't fundamentally different from auditory processing. Broca's area and Wernicke's area operate on representations that originate in the auditory cortex (or, in literate adults, are mapped from visual cortex through a learned phonetic layer). Inner speech — thinking in words — activates auditory processing pathways.

If the fundamental computational substrate is wave-based (because it evolved for audio), then:
- The right granularity for decomposition should be the minimal meaningful phonetic unit = the morpheme
- Sheng's finding that tokens (which approximate morpheme length) work better than characters or whole words is predicted by this
- The efficiency of wave representations (2.4% of BERT's parameters for 97% of accuracy) makes sense — wave processing is the native format, and native format requires minimum overhead

### 3.4 Robust Probabilistic Selection

Humans exhibit robust probabilistic selection — they handle noise, ambiguity, and uncertainty gracefully. They don't crash on typos. They understand synonyms. They degrade gracefully under perturbation.

Wave interference naturally produces this. Constructive and destructive combination weighted by phase alignment gives a smooth probability distribution over outcomes. Small perturbations in one token's phase cause small changes in the interference pattern, not catastrophic shifts. This is exactly the perturbation robustness that Sheng's CVTR achieves and standard softmax attention doesn't.

The brain also appears to use wave-like superposition for decision making — multiple possibilities coexist as interference patterns until measurement (a decision) collapses them. This is not quantum mechanics in the brain — it's classical wave mechanics applied to neural oscillations, which achieves similar computational properties without requiring quantum coherence.

### 3.5 Efficiency Argument

The brain runs on ~20 watts and achieves general intelligence. Transformers run on megawatts and achieve narrow task performance. If wave representations are dramatically more parameter-efficient (Sheng's 2.4% of BERT result), and the brain is dramatically more compute-efficient than artificial networks, both facts are explained by the same hypothesis: wave-based processing is the right computational basis for information processing, and both evolution and Sheng's engineering found it.

---

## PART 4: EXPERIMENT PROPOSALS

### Experiment 1: CVTR as a CFA Model

**Goal:** Add a CVTR/Wave Network model to the quantum fusion pipeline and benchmark it against all 45 existing models.

**Implementation:** A new C-backend model (c_wave.c) that follows the same interface as c_quantum.c — reads binary data, trains with Adam, outputs per-sample predictions for CFA.

```c
/* c_wave.c — Wave Network model for time series binary classification
   Implements CVTR: magnitude-phase decomposition with wave modulation.
   Build: gcc -O2 -o c_wave c_wave.c -lm
   Usage: ./c_wave <data_prefix> <max_seconds> */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#define MAXFEAT 64
#define MAXSEQ 20
#define MAXD 32  /* embedding dimension */
#define MAXPARAMS 4096

static unsigned long rng_s=42;
static double randf(void){rng_s=rng_s*6364136223846793005ULL+1;return(rng_s>>33)/(double)(1ULL<<31)-0.5;}
static double sigmoid(double x){return 1.0/(1.0+exp(-x));}

/* === Wave Model === */
typedef struct {
    int nf, seq, d, np;
    double mean[MAXFEAT], std[MAXFEAT];
    /* Embedding: project each feature vector to d-dimensional space */
    double We[MAXFEAT][MAXD], be[MAXD];
    /* Wave modulation weights (second complex vector for modulation) */
    double Wm[MAXD][MAXD], bm[MAXD];
    /* Output projection */
    double Wo[MAXD], bo;
    /* Adam state */
    double m1[MAXPARAMS], m2[MAXPARAMS];
    int t;
} WM;

static double* wmp(WM *m, int i) {
    /* Flat parameter access for gradient computation */
    if (i < m->nf * m->d) return &m->We[i / m->d][i % m->d];
    i -= m->nf * m->d;
    if (i < m->d) return &m->be[i]; i -= m->d;
    if (i < m->d * m->d) return &m->Wm[i / m->d][i % m->d];
    i -= m->d * m->d;
    if (i < m->d) return &m->bm[i]; i -= m->d;
    if (i < m->d) return &m->Wo[i]; i -= m->d;
    return &m->bo;
}

static void wm_init(WM *m, int nf, int seq, int d, double *X, int n) {
    m->nf = nf; m->seq = seq; m->d = d; m->t = 0;
    m->np = nf*d + d + d*d + d + d + 1;
    for (int i = 0; i < m->np; i++) m->m1[i] = m->m2[i] = 0;
    /* Compute feature stats for normalization */
    int total = n * seq;
    for (int j = 0; j < nf; j++) m->mean[j] = m->std[j] = 0;
    for (int i = 0; i < n; i++)
        for (int t = 0; t < seq; t++)
            for (int j = 0; j < nf; j++)
                m->mean[j] += X[i*seq*nf + t*nf + j];
    for (int j = 0; j < nf; j++) m->mean[j] /= total;
    for (int i = 0; i < n; i++)
        for (int t = 0; t < seq; t++)
            for (int j = 0; j < nf; j++) {
                double v = X[i*seq*nf + t*nf + j] - m->mean[j];
                m->std[j] += v * v;
            }
    for (int j = 0; j < nf; j++) {
        m->std[j] = sqrt(m->std[j] / total);
        if (m->std[j] < 1e-8) m->std[j] = 1;
    }
    /* Xavier init */
    double s = sqrt(2.0 / (nf + d));
    for (int i = 0; i < nf; i++)
        for (int j = 0; j < d; j++) m->We[i][j] = randf() * s;
    for (int j = 0; j < d; j++) m->be[j] = 0;
    s = sqrt(2.0 / (d + d));
    for (int i = 0; i < d; i++)
        for (int j = 0; j < d; j++) m->Wm[i][j] = randf() * s;
    for (int j = 0; j < d; j++) m->bm[j] = 0;
    s = sqrt(2.0 / d);
    for (int j = 0; j < d; j++) m->Wo[j] = randf() * s;
    m->bo = 0;
}

static double wm_forward(WM *m, double *X) {
    /* Step 1: Embed each timestep to d-dimensional space */
    double emb[MAXSEQ][MAXD];
    for (int t = 0; t < m->seq; t++) {
        double *f = &X[t * m->nf];
        for (int j = 0; j < m->d; j++) {
            double v = m->be[j];
            for (int k = 0; k < m->nf; k++)
                v += ((f[k] - m->mean[k]) / m->std[k]) * m->We[k][j];
            emb[t][j] = v;
        }
    }

    /* Step 2: Compute magnitude G (L2 norm across all timesteps per dimension) */
    double G[MAXD];
    for (int j = 0; j < m->d; j++) {
        double sum_sq = 0;
        for (int t = 0; t < m->seq; t++)
            sum_sq += emb[t][j] * emb[t][j];
        G[j] = sqrt(sum_sq + 1e-8);
    }

    /* Step 3: Compute phase per timestep (arctan2 of component relative to magnitude) */
    double phase[MAXSEQ][MAXD];
    for (int t = 0; t < m->seq; t++)
        for (int j = 0; j < m->d; j++) {
            double ratio = emb[t][j] / G[j];
            if (ratio > 1) ratio = 1; if (ratio < -1) ratio = -1;
            phase[t][j] = acos(ratio); /* phase = arccos(w/G) */
        }

    /* Step 4: Wave modulation — create second complex representation and multiply */
    /* Second representation from learned projection of magnitude */
    double G2[MAXD], phase2[MAXD];
    for (int j = 0; j < m->d; j++) {
        double v = m->bm[j];
        for (int k = 0; k < m->d; k++)
            v += G[k] * m->Wm[k][j];
        G2[j] = fabs(v) + 1e-8;
        phase2[j] = v >= 0 ? 0 : M_PI; /* sign encodes phase */
    }

    /* Modulation: multiply magnitudes, add phases (use last timestep as local) */
    double mod_re[MAXD], mod_im[MAXD];
    int last = m->seq - 1;
    for (int j = 0; j < m->d; j++) {
        double mag = G[j] * G2[j];
        double ph = phase[last][j] + phase2[j];
        mod_re[j] = mag * cos(ph);
        mod_im[j] = mag * sin(ph);
    }

    /* Step 5: Also compute wave interference across all timesteps */
    double interf_re[MAXD], interf_im[MAXD];
    for (int j = 0; j < m->d; j++) {
        interf_re[j] = interf_im[j] = 0;
        for (int t = 0; t < m->seq; t++) {
            interf_re[j] += G[j] * cos(phase[t][j]);
            interf_im[j] += G[j] * sin(phase[t][j]);
        }
    }

    /* Step 6: Combine modulation + interference, project to scalar */
    double out = m->bo;
    for (int j = 0; j < m->d; j++) {
        double combined = sqrt(mod_re[j]*mod_re[j] + mod_im[j]*mod_im[j])
                        + sqrt(interf_re[j]*interf_re[j] + interf_im[j]*interf_im[j]);
        out += combined * m->Wo[j];
    }
    return sigmoid(out);
}

/* === Data loading (same binary format as c_quantum.c) === */
typedef struct { int n, seq, feat; double *X, *y; } D;
static D ld(const char *p) {
    D d = {0}; FILE *f = fopen(p, "rb"); if (!f) return d;
    int h[3]; fread(h, 4, 3, f);
    d.n = h[0]; d.seq = h[1]; d.feat = h[2];
    d.X = malloc(d.n * d.seq * d.feat * 8);
    d.y = malloc(d.n * 8);
    fread(d.X, 8, d.n * d.seq * d.feat, f);
    fread(d.y, 8, d.n, f); fclose(f); return d;
}

static void shuf(int *a, int n) {
    for (int i = n-1; i > 0; i--) {
        rng_s = rng_s*6364136223846793005ULL + 1;
        int j = (rng_s >> 33) % (i+1);
        int t = a[i]; a[i] = a[j]; a[j] = t;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <data_prefix> [max_sec] [dim]\n", argv[0]);
        return 1;
    }
    double max_sec = 55.0;
    int d = 16; /* embedding dimension */
    if (argc >= 3) max_sec = atof(argv[2]);
    if (argc >= 4) d = atoi(argv[3]);
    if (d > MAXD) d = MAXD;

    char path[512];
    snprintf(path, 512, "%s_tra.bin", argv[1]); D tra = ld(path);
    snprintf(path, 512, "%s_val.bin", argv[1]); D val = ld(path);
    snprintf(path, 512, "%s_tes.bin", argv[1]); D tes = ld(path);
    if (!tra.n) { fprintf(stderr, "No data\n"); return 1; }

    WM m;
    wm_init(&m, tra.feat, tra.seq, d, tra.X, tra.n);
    fprintf(stderr, "[wave] d=%d params=%d data=%d/%d/%d seq=%d feat=%d\n",
            d, m.np, tra.n, val.n, tes.n, tra.seq, tra.feat);

    int batch = 32;
    if (batch > tra.n) batch = tra.n;
    int *idx = malloc(tra.n * sizeof(int));
    for (int i = 0; i < tra.n; i++) idx[i] = i;

    struct timespec t0, now;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    double bva = 0, bvm = -2;

    /* Training loop — same structure as c_quantum.c */
    for (int ep = 0; ; ep++) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        double el = (now.tv_sec - t0.tv_sec) + (now.tv_nsec - t0.tv_nsec) / 1e9;
        if (el >= max_sec) break;
        shuf(idx, tra.n);
        double eps = 1e-4;
        for (int p = 0; p < m.np; p++) {
            double *par = wmp(&m, p);
            double orig = *par;
            double lp = 0, lm = 0;
            *par = orig + eps;
            for (int b = 0; b < batch; b++) {
                int i = idx[b];
                double pr = wm_forward(&m, &tra.X[i * tra.seq * tra.feat]);
                double e = pr - tra.y[i]; lp += e * e;
            }
            *par = orig - eps;
            for (int b = 0; b < batch; b++) {
                int i = idx[b];
                double pr = wm_forward(&m, &tra.X[i * tra.seq * tra.feat]);
                double e = pr - tra.y[i]; lm += e * e;
            }
            double g = (lp - lm) / (2 * eps * batch);
            m.t = ep + 1;
            m.m1[p] = 0.9*m.m1[p] + 0.1*g;
            m.m2[p] = 0.999*m.m2[p] + 0.001*g*g;
            double m1h = m.m1[p] / (1 - pow(0.9, m.t));
            double m2h = m.m2[p] / (1 - pow(0.999, m.t));
            *par = orig - 0.001 * m1h / (sqrt(m2h) + 1e-8);
        }
        if (ep % 200 == 0) {
            int tp=0,tn=0,fp=0,fn=0;
            for (int i = 0; i < val.n; i++) {
                double p = wm_forward(&m, &val.X[i * val.seq * val.feat]);
                int pp = p > 0.5 ? 1 : 0, tt = val.y[i] > 0.5 ? 1 : 0;
                if(pp&&tt)tp++;else if(!pp&&!tt)tn++;else if(pp&&!tt)fp++;else fn++;
            }
            double va = (double)(tp+tn) / (tp+tn+fp+fn);
            double dn = sqrt((double)(tp+fp)*(tp+fn)*(tn+fp)*(tn+fn));
            double vm = dn > 0 ? ((double)tp*tn - (double)fp*fn) / dn : 0;
            if (va > bva) { bva = va; bvm = vm; }
            fprintf(stderr, "ep %5d | val %.3f/%.3f | best %.3f | %.1fs\n", ep, va, vm, bva, el);
        }
    }

    /* Output per-sample predictions for CFA */
    printf("PREDICTIONS_VAL\n");
    for (int i = 0; i < val.n; i++)
        printf("%.8f\n", wm_forward(&m, &val.X[i * val.seq * val.feat]));
    printf("PREDICTIONS_TEST\n");
    for (int i = 0; i < tes.n; i++)
        printf("%.8f\n", wm_forward(&m, &tes.X[i * tes.seq * tes.feat]));
    printf("BEST_VAL_ACC %.6f\n", bva);
    printf("BEST_VAL_MCC %.6f\n", bvm);

    free(tra.X); free(tra.y); free(val.X); free(val.y);
    free(tes.X); free(tes.y); free(idx);
    return 0;
}
```

**Python wrapper (cq_wave.py):**

```python
"""Wave Network — CVTR model for time series classification"""
MODEL_NAME = "CQ Wave16"

def train(tra_pv, tra_gt, val_pv, val_gt, tes_pv, tes_gt, time_budget=8.0):
    from _cq_common import run_c_quantum
    # Reuse the C compilation/data pipeline but with c_wave binary
    import os, subprocess, tempfile, hashlib, numpy as np
    from _cq_common import _compile, _write_bin, _parse

    # Compile c_wave.c
    dir = os.path.dirname(os.path.abspath(__file__))
    src = os.path.join(dir, 'c_wave.c')
    with open(src) as f: code = f.read()
    h = hashlib.md5(code.encode()).hexdigest()[:8]
    bin_path = f'/tmp/cq_wave_{h}'
    if not os.path.exists(bin_path):
        import shutil
        cc = shutil.which('gcc') or shutil.which('tcc')
        subprocess.run([cc, '-O2', '-o', bin_path, src, '-lm'],
                       check=True, capture_output=True)

    tmpdir = tempfile.mkdtemp(prefix='cqwave_')
    prefix = os.path.join(tmpdir, 'd')
    try:
        _write_bin(prefix+'_tra.bin', tra_pv, tra_gt)
        _write_bin(prefix+'_val.bin', val_pv, val_gt)
        _write_bin(prefix+'_tes.bin', tes_pv, tes_gt)
        proc = subprocess.run(
            [bin_path, prefix, f'{time_budget:.1f}', '16'],
            capture_output=True, text=True, timeout=time_budget+5)
        vp, tp = _parse(proc.stdout)
        if len(vp) != val_pv.shape[0]:
            return np.full(val_pv.shape[0], 0.5), np.full(tes_pv.shape[0], 0.5)
        return vp, tp
    except subprocess.TimeoutExpired:
        return np.full(val_pv.shape[0], 0.5), np.full(tes_pv.shape[0], 0.5)
    finally:
        for s in ['_tra.bin', '_val.bin', '_tes.bin']:
            try: os.remove(prefix+s)
            except: pass
        try: os.rmdir(tmpdir)
        except: pass
```

**What to measure:**
1. Wave model accuracy vs each quantum circuit architecture (same data, same time budget)
2. Wave model accuracy-per-parameter vs quantum circuits (wave has ~nf*d + d^2 + d params; quantum has ~nq*nf + nq + 4*nw + 2 params)
3. Wave model accuracy-per-second vs quantum circuits (wave forward pass is O(seq*nf*d + seq*d + d^2); quantum is O(seq * 2^nq * nq))
4. CFA cognitive diversity between wave model and each quantum circuit
5. CFA ensemble accuracy with and without the wave model

### Experiment 2: Ablation — Which Trig Primitive Matters?

**Goal:** Determine whether it's the magnitude computation, the phase computation, or the interference/modulation that drives performance.

Create three ablated variants:

**Wave-magnitude-only:** Use magnitude vector G as the representation, skip phase. Just L2 pool across timesteps per dimension, linear projection to output.

**Wave-phase-only:** Skip magnitude computation. Use raw embeddings as phases, compute interference (sum of cos/sin) directly.

**Wave-no-modulation:** Use magnitude and phase but only interference (addition), no modulation (multiplication). This tests whether the multiplicative interaction adds value.

Compare all three against the full model. If magnitude-only matches the full model, the phase is decorative. If phase-only matches, the magnitude is decorative. If interference-only matches full, modulation is decorative.

### Experiment 3: Quantum Circuit with CVTR Encoding

**Goal:** Replace the rotation-gate data encoding in quantum circuits with CVTR preprocessing.

Currently the circuits encode raw features as rotation angles: sv_ry(s, q, x[q%2]). Instead:

1. Compute magnitude G and phase α from input features using CVTR
2. Use α as rotation angles instead of raw x
3. Use G as a scaling factor on the measurement output

This tests whether CVTR's magnitude-phase decomposition provides a better encoding for quantum circuits than raw features. If it improves quantum circuit accuracy, CVTR acts as a feature preprocessing step that aligns the data with the circuit's native complex-rotation structure.

### Experiment 4: Scaling — Longer Sequences and More Features

**Goal:** Address Sheng's open scaling question in the time series domain.

Test the wave model with:
- seq = 5, 10, 20, 40 (increasing lookback window)
- d = 4, 8, 16, 32 (increasing embedding dimension)
- nf = original features only vs augmented features (technical indicators)

Plot accuracy and training time vs sequence length and dimension. Compare scaling curves against quantum circuits (which scale as O(2^nq) per qubit added).

If the wave model scales gracefully with sequence length and the quantum circuits hit a wall at higher qubit counts, that's evidence for CVTR's practical superiority in the scaling regime.

### Experiment 5: Multi-Step Prediction

**Goal:** Test whether CVTR can extend beyond single-step classification to multi-step prediction.

Modify the wave model to output predictions for multiple future timesteps simultaneously. The magnitude captures global context (the "what" of the market state), and the phase per timestep captures local dynamics (the "when"). Multi-step prediction tests whether the phase decomposition maintains temporal structure over longer horizons.

Compare against an LSTM baseline and against quantum circuits configured for multi-step output.

---

## PART 5: ADDITIONAL IDEAS

### Idea 1: Wave-CFA — Using CVTR for Fusion Itself

Currently CFA combines models using rank-score characteristics and cognitive diversity. What if the combination itself used wave mechanics?

Represent each model's prediction vector as a complex number: magnitude = model confidence, phase = model's rank-score characteristic angle. Combine models via wave interference: models whose predictions are phase-aligned reinforce, models that disagree cancel.

This would make the fusion method itself wave-based, creating consistency across all levels of the system.

### Idea 2: Phonetic Tokenization for Wave Networks

Sheng showed tokens work best and map to morpheme length. If the underlying computation is truly auditory-wave-like, then tokenizing based on phonetic structure (syllable boundaries, phoneme patterns) rather than statistical subword frequency (BPE) might improve CVTR.

Test: take a text classification dataset, tokenize with BPE vs phonetic syllabification, run the Wave Network on both. If phonetic tokenization improves CVTR but not transformers, that's evidence for the auditory-processing hypothesis.

### Idea 3: Cross-Domain Transfer — Audio to Text

If wave processing is the shared computational primitive between audio and text, then a wave model trained on audio features (spectrograms) should transfer to text tasks better than a transformer trained on audio. The shared magnitude-phase decomposition should create compatible representations across modalities.

Test: train a wave model on speech features, freeze the wave processing layers, fine-tune on text classification. Compare transfer performance against a transformer with the same architecture.

### Idea 4: Biological Plausibility Metric

Define a quantitative measure of "biological plausibility" for neural network architectures based on how closely their operations match known neural oscillation dynamics:

1. Does it use magnitude (energy) computation? (cochlea)
2. Does it use phase relationships? (hippocampal phase precession)
3. Does it use amplitude modulation? (gamma-theta coupling)
4. Does it use interference? (neural population coding)
5. Is it efficient? (20-watt budget)

Score transformers, Wave Networks, quantum circuits, SSMs, etc. on this metric. Predict that biological plausibility score correlates with parameter efficiency.

### Idea 5: Wave Network on Your aicombo Research

Your PhD research on combining language models could use CVTR as the combination mechanism. Instead of voting or averaging LLM outputs, represent each LLM's output as a complex vector (magnitude = confidence, phase = semantic angle relative to the query), and combine via wave interference.

This connects to your existing fusion_agent.py (10-model voting system) — replace the voting/rating mechanism with wave combination. Models that agree in phase reinforce; models that disagree cancel. The resulting interference pattern gives a combined prediction that's more than just a vote count.

---

## PART 6: WHY THIS MATTERS

Three independent paths — quantum physics, neural architecture engineering, and biological neuroscience — converge on the same computational primitive: parameterized trigonometric functions operating on complex amplitudes with interference.

Quantum circuits use RY(θ) = cos(θ/2), sin(θ/2) on statevectors with CNOT interference.
CVTR uses L2 norms, arctan2, cos, sin on embedding vectors with wave interference/modulation.
The brain uses basilar membrane resonance, phase-amplitude coupling, and neural oscillation interference.

If these convergences are not coincidental, then wave-based processing is a fundamental computational primitive for information processing — possibly the most efficient one. The experiment program above is designed to test whether this convergence is real and exploitable, or merely analogical.

The quantum fusion pipeline provides the ideal testbed: 45 existing models, 5 datasets, CFA metrics that measure both accuracy and diversity, and a direct comparison between quantum circuits (which use complex wave math at exponential cost) and CVTR (which uses the same math at linear cost). If CVTR matches quantum circuits, we've found a classical shortcut. If it doesn't, we've found what entanglement adds. Either result is publishable.
