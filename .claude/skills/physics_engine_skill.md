<skill>
  <name>Quantum Physics Engine Architect</name>
  <description>Authority on hydrogenic wavefunctions, matrix mechanics, and relativistic quantum corrections for real-time C++ simulation.</description>
  <instructions>
    <rule>Mathematical Ground Truth: All calculations must use double-precision floating point. No hard-coded orbital radii.</rule>
    <rule>Hydrogenic Basis: Use the general solution for the Schrodinger equation in a central potential: $\psi_{nlm}(r, \theta, \phi) = R_{nl}(r) Y_{lm}(\theta, \phi)$.</rule>
    <rule>Normalization: Every wavefunction generated must be normalized such that $\int |\psi|^2 d\tau = 1$.</rule>
    <rule>Relativistic Constraints: For high-Z atoms or precise transitions, implement the Fine Structure constant $\alpha \approx 1/137.036$ corrections.</rule>
    <rule>Complex Numbers: All quantum mechanical calculations must use std::complex<double>. Cast to float only at the rendering boundary.</rule>
    <rule>Time Dependence: For time-dependent problems, use $i\hbar\frac{\partial}{\partial t}\psi = \hat{H}\psi$.</rule>

    <concepts>
      <concept name="Radial Wavefunction">
        Equation: $R_{nl}(r) = \sqrt{(\frac{2Z}{na_0})^3 \frac{(n-l-1)!}{2n[(n+l)!]}} e^{-\rho/2} \rho^l L_{n-l-1}^{2l+1}(\rho)$
        Where $\rho = \frac{2Zr}{na_0}$ and $L$ are Associated Laguerre Polynomials.
      </concept>
      <concept name="Angular Component">
        Use Spherical Harmonics $Y_{lm}(\theta, \phi)$ to define the orbital shape (s, p, d, f).
      </concept>
      <concept name="Energy Eigenvalues">
        Non-relativistic: $E_n = -\frac{m_e e^4 Z^2}{8 \epsilon_0^2 h^2 n^2} \approx -13.6 \frac{Z^2}{n^2} \text{ eV}$.
      </concept>
      <concept name="Dirac Equation (Spin)">
        Incorporate electron spin $s = \pm 1/2$ and the resulting split in energy levels (Fine Structure).
      </concept>
      <concept name="Time-Dependent Schrodinger Equation (TDSE)">
        The TDSE describes the time evolution of a quantum state:
        $$i\hbar\frac{\partial\psi(x,t)}{\partial t} = \left[-\frac{\hbar^2}{2m}\frac{\partial^2}{\partial x^2} + V(x,t)\right]\psi(x,t)$$
        For a free particle ($V=0$), the solution is a plane wave $\psi(x,t) = A e^{i(kx - \omega t)}$.
        The dispersion relation is $\omega = \frac{\hbar k^2}{2m}$.
      </concept>
      <concept name="Pauli Spin Matrices">
        The Pauli matrices form a basis for spin-1/2 operators:
        $$\sigma_x = \begin{pmatrix} 0 & 1 \\ 1 & 0 \end{pmatrix}, \quad
        \sigma_y = \begin{pmatrix} 0 & -i \\ i & 0 \end{pmatrix}, \quad
        \sigma_z = \begin{pmatrix} 1 & 0 \\ 0 & -1 \end{pmatrix}$$
        They satisfy $\sigma_i\sigma_j = \delta_{ij}I + i\epsilon_{ijk}\sigma_k$.
      </concept>
      <concept name="Dirac Notation (Bra-Ket)">
        Quantum states are represented as ket vectors: $|\psi\rangle$
        The dual (bra) is the conjugate transpose: $\langle\psi|$
        Inner product: $\langle\phi|\psi\rangle$ (scalar)
        Outer product: $|\phi\rangle\langle\psi|$ (operator)
        Expectation value: $\langle A \rangle = \langle\psi|A|\psi\rangle$
      </concept>
      <concept name="Gaussian Wave Packet">
        A localized wave packet for wave-particle duality:
        $$\psi(x,t) = A \cdot \exp\left(-\frac{(x - x_0 - v_g t)^2}{4\sigma^2}\right) \cdot \exp(i(k_0 x - \omega t))$$
        Where $v_g = \frac{\partial\omega}{\partial k} = \frac{\hbar k_0}{m}$ is the group velocity.
        The probability density is $|\psi|^2 = |A|^2 \exp\left(-\frac{(x - x_0 - v_g t)^2}{2\sigma^2}\right)$.
      </concept>
      <concept name="Wave Packet Propagation">
        For superposition of two wave packets $\psi = \psi_1 + \psi_2$:
        $$|\psi|^2 = |\psi_1|^2 + |\psi_2|^2 + 2\Re(\psi_1^*\psi_2)$$
        The interference term $2\Re(\psi_1^*\psi_2)$ can be constructive (positive) or destructive (negative).
      </concept>
      <concept name="Bloch Sphere">
        Any pure spin-1/2 state can be represented on the Bloch sphere:
        $$|\psi\rangle = \cos(\theta/2)|0\rangle + e^{i\phi}\sin(\theta/2)|1\rangle$$
        Where $|0\rangle = \begin{pmatrix}1\\0\end{pmatrix}$ and $|1\rangle = \begin{pmatrix}0\\1\end{pmatrix}$.
        The spin projection along $\hat{n}$ is $\langle\vec{S}\cdot\hat{n}\rangle = \frac{\hbar}{2}(\sin\theta\cos\phi, \sin\theta\sin\phi, \cos\theta)$.
      </concept>
    </concepts>

    <implementation_mandates>
      <mandate>Use a recursive algorithm for Laguerre Polynomials to avoid factorial overflow at high $n$.</mandate>
      <mandate>Implement the Rydberg Formula for photon $\lambda$: $\frac{1}{\lambda} = R_H Z^2 (\frac{1}{n_{final}^2} - \frac{1}{n_{initial}^2})$.</mandate>
      <mandate>Probabilistic Sampling: For point-cloud rendering, use the Metropolis-Hastings algorithm to sample positions from the probability density $|\psi|^2$.</mandate>
      <mandate>Wave Packet Implementation: Gaussian wave packets must include both envelope and phase components using std::complex<double>.</mandate>
      <mandate>Spin System: Pauli matrices must be implemented as 2x2 matrices of std::complex<double>.</mandate>
    </implementation_mandates>

    <example>
      // Correct: Recursive Laguerre calculation for stability
      double AssociatedLaguerre(int n, int k, double x) {
          if (n == 0) return 1.0;
          if (n == 1) return 1.0 + k - x;
          return ((2 * n - 1 + k - x) * AssociatedLaguerre(n - 1, k, x)
                 - (n - 1 + k) * AssociatedLaguerre(n - 2, k, x)) / n;
      }
    </example>

    <example>
      // Correct: Gaussian wave packet evaluation
      std::complex<double> GaussianWavePacket(double x, double t, double x0, double v, double sigma, double k0, double omega) {
          double x_center = x0 + v * t;
          double envelope = std::exp(-std::pow(x - x_center, 2) / (4.0 * sigma * sigma));
          std::complex<double> phase = std::exp(std::complex<double>(0.0, k0 * x - omega * t));
          return envelope * phase;
      }
    </example>

    <example>
      // Correct: Pauli matrix sigma_x
      std::complex<double> sigma_x[2][2] = {
          {0, 1},
          {1, 0}
      };
      // Using std::complex for proper quantum mechanics
    </example>

    <anti_example>
      // WRONG: Using a simple sine wave to approximate an orbital
      float orbit = sin(time * energy) * radius;
    </anti_example>
    <anti_example>
      // WRONG: Using float for quantum calculations
      float complex_number; // Must use double
    </anti_example>
  </instructions>
</skill>
