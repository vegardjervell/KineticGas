/*
Author: Vegard Gjeldvik Jervell
Contains: The MieKinGas class. This class is the model used to evaluate the Enskog solutions for a Mie potential.
            MieKinGas overrides the potential and potential derivative functions in Spherical.
            MieKinGas is inherited by QuantumMie.
            Also contains functions required to compute the radial distribution function at contact for a Mie-fluid
            and related constant factors that have been previously regressed (namespace mie_rdf_constants).
            See : J. Chem. Phys. 139, 154504 (2013); https://doi.org/10.1063/1.4819786
*/

#pragma once
#include "Spherical.h"

class MieKinGas : public Spherical {
    public:
    std::vector<std::vector<double>> eps, la, lr, C, alpha;

    MieKinGas(std::vector<double> mole_weights,
        std::vector<std::vector<double>> sigmaij,
        std::vector<std::vector<double>> eps,
        std::vector<std::vector<double>> la,
        std::vector<std::vector<double>> lr,
        bool is_idealgas)
        : Spherical(mole_weights, sigmaij, is_idealgas),
        eps{eps},
        la{la},
        lr{lr}
        {

        #ifdef DEBUG
            std::printf("This is a Debug build!\nWith, %E, %E\n\n", mole_weights[0], mole_weights[1]);
        #endif

        C = std::vector<std::vector<double>>(Ncomps, std::vector<double>(Ncomps, 0.));
        alpha = std::vector<std::vector<double>>(Ncomps, std::vector<double>(Ncomps, 0.));
        for (int i = 0; i < eps.size(); i ++){
            for (int j = 0; j < eps.size(); j++){
                C[i][j] = (lr[i][j] / (lr[i][j] - la[i][j])) 
                                * pow(lr[i][j] / la[i][j], (la[i][j] / (lr[i][j] - la[i][j])));
                alpha[i][j] = C[i][j] * ((1.0 / (la[i][j] - 3.0)) - (1.0 / (lr[i][j] - 3.0)));
            }
        }   
    }

    double potential(int i, int j, double r) override {
        return C[i][j] * eps[i][j] * (pow(sigma[i][j] / r, lr[i][j]) - pow(sigma[i][j] / r, la[i][j]));
    }
    double potential_derivative_r(int i, int j, double r) override {
        return C[i][j] * eps[i][j] * ((la[i][j] * pow(sigma[i][j], la[i][j]) / pow(r, la[i][j] + 1)) 
                                        - (lr[i][j] * pow(sigma[i][j], lr[i][j]) / pow(r, lr[i][j] + 1)));
    }

    double potential_dblderivative_rr(int i, int j, double r) override {
        return C[i][j] * eps[i][j] * ((lr[i][j] * (lr[i][j] + 1) * pow(sigma[i][j], lr[i][j]) / pow(r, lr[i][j] + 2)) 
                                    - (la[i][j] * (la[i][j] + 1) * pow(sigma[i][j], la[i][j]) / pow(r, la[i][j] + 2)));
    }


    double omega(const int& i, const int& j, const int& l, const int& r, const double& T) override;
    double omega_correlation(int i, int j, int l, int r, double T_star);
    double omega_recursive_factor(int i, int j, int l, int r, double T);
    // The hard sphere integrals are used as the reducing factor for the correlations.
    // So we need to compute the hard-sphere integrals to convert the reduced collision integrals from the
    // Correlation by Fokin et. al. to the "real" collision integrals.
    inline double omega_hs(const int& i, const int& j, const int& l, const int& r, const double& T){
        double w = PI * pow(sigma[i][j], 2) * 0.5 * (r + 1);
        for (int ri = r; ri > 1; ri--) {w *= ri;}
        if (l % 2 == 0){
            w *= (1. - (1.0 / (l + 1.)));
        }
        if (i == j) return sqrt((BOLTZMANN * T) / (PI * m[i])) * w;
        return sqrt(BOLTZMANN * T * (m[i] + m[j]) / (2. * PI * m[i] * m[j])) * w;
    }

    static constexpr double omega_correlation_factors[2][6][4] =
        {
         {
          {0., -0.145269e1, 0.294682e2, 0.242508e1},
          {0.107782e-1, 0.587725, -0.180714e3, .595694e2},
          {0.546646e-1, -0.651465e1, 0.374457e3, -0.137807e3},
          {0.485352, 0.245523e2, -0.336782e3, 0.814187e2},
          {-0.385355, -0.206868e2, 0.132246e3, 0.},
          {0.847232e-1, 0.521812e1, -0.181140e2, -0.747215e1}
         },
         {
          {0., 0.113086e1, 0.234799e2, 0.310127e1},
          {0., 0.551559e1, -0.137023e3, 0.185848e2},
          {0.325909e-1, -0.292925e2, 0.243761e3, 0.},
          {0.697682, 0.590792e2, -0.143670e3, -0.123518e3},
          {-0.564238, -0.430549e2, 0., 0.137282e3},
          {0.126508, 0.104273e2, 0.150601e2, -0.408911e2}
         }
        };

    // Contact diameter related methods
    // bmax[i][j] is in units of sigma[i][j]
    // bmax = The maximum value of the impact parameter at which deflection angle (chi) is positive
    std::vector<std::vector<double>> get_b_max(double T);
    std::vector<std::vector<double>> get_contact_diameters(double rho, double T, const std::vector<double>& x) override;
    virtual std::vector<std::vector<double>> get_BH_diameters(double T);

    // Methods for computing the radial distribution function at contact
    std::vector<std::vector<double>> model_rdf(double rho, double T, const std::vector<double>& x) override;
    std::vector<std::vector<double>> rdf_HS(double rho, double T, const std::vector<double>& x);
    std::vector<std::vector<double>> rdf_g1_func(double rho, const std::vector<double>& x,
                                                const std::vector<std::vector<double>>& d_BH);
    std::vector<std::vector<double>> rdf_g1_func(double rho, double T, const std::vector<double>& x);
    std::vector<std::vector<double>> rdf_g2_func(double rho, double T, const std::vector<double>& x,
                                                const std::vector<std::vector<double>>& d_BH,
                                                const std::vector<std::vector<double>>& x0);
    std::vector<std::vector<double>> rdf_g2_func(double rho, double T, const std::vector<double>& x);
    std::vector<std::vector<double>> get_x0(const std::vector<std::vector<double>>& d_BH);
    std::vector<std::vector<double>> a_1s_func(double rho, const std::vector<double>& x,
                                                const std::vector<std::vector<double>>& d_BH,
                                                const std::vector<std::vector<double>>& lambda);
    std::vector<std::vector<double>> a_1s_func(double rho, double T, const std::vector<double>& x,
                                                const std::vector<std::vector<double>>& lambda);
    std::vector<std::vector<double>> da1s_drho_func(double rho, const std::vector<double>& x,
                                                    const std::vector<std::vector<double>>& d_BH,
                                                    const std::vector<std::vector<double>>& lambda);
    std::vector<std::vector<double>> da1s_drho_func(double rho, double T, const std::vector<double>& x,
                                                const std::vector<std::vector<double>>& lambda);

    std::vector<std::vector<double>> I_func(const std::vector<std::vector<double>>& x0,
                                            const std::vector<std::vector<double>>& lambda);
    std::vector<std::vector<double>> J_func(const std::vector<std::vector<double>>& x0,
                                            const std::vector<std::vector<double>>& lambda);
    std::vector<std::vector<double>> B_func(double rho, const std::vector<double>& x,
                                            const std::vector<std::vector<double>>& d_BH,
                                            const std::vector<std::vector<double>>& lambda);
    std::vector<std::vector<double>> dBdrho_func(double rho, const std::vector<double>& x,
                                                const std::vector<std::vector<double>>& d_BH,
                                                const std::vector<std::vector<double>>& lambda);

    std::vector<std::vector<double>> a1ij_func(double rho, double T, const std::vector<double>& x);

    std::vector<std::vector<double>> da1ij_drho_func(double rho, const std::vector<double>& x,
                                                    const std::vector<std::vector<double>>& d_BH);
    std::vector<std::vector<double>> da1ij_drho_func(double rho, double T, const std::vector<double>& x);                                                
    
    std::vector<std::vector<double>> a2ij_func(double rho, const std::vector<double>& x, double K_HS,
                                                const std::vector<std::vector<double>>& rdf_chi,
                                                const std::vector<std::vector<double>>& d_BH,
                                                const std::vector<std::vector<double>>& x0);
    std::vector<std::vector<double>> a2ij_func(double rho, double T, const std::vector<double>& x);

    std::vector<std::vector<double>> da2ij_drho_func(double rho, const std::vector<double>& x, double K_HS, 
                                                    const std::vector<std::vector<double>>& d_BH,
                                                    const std::vector<std::vector<double>>& x0);
    std::vector<std::vector<double>> da2ij_drho_func(double rho, double T, const std::vector<double>& x);

    std::vector<std::vector<double>> da2ij_div_chi_drho_func(double rho, const std::vector<double>& x, double K_HS, 
                                                const std::vector<std::vector<double>>& d_BH,
                                                const std::vector<std::vector<double>>& x0);

    std::vector<std::vector<double>> rdf_chi_func(double rho, const std::vector<double>& x,
                                                const std::vector<std::vector<double>>& d_BH);
    std::vector<std::vector<double>> drdf_chi_drho_func(double rho, const std::vector<double>& x,
                                                    const std::vector<std::vector<double>>& d_BH);
    
    std::vector<std::vector<double>> gamma_corr(double zeta_x, double T);

    inline double K_HS_func(double zeta_x){
        return pow(1 - zeta_x, 4) / (1 + 4 * zeta_x + 4 * pow(zeta_x, 2) - 4 * pow(zeta_x, 3) + pow(zeta_x, 4));
    }
    inline double dKHS_drho_func(double zeta_x, double dzx_drho){
        return - 4 * dzx_drho * pow(1 - zeta_x, 3) * (2 + 5 * zeta_x - pow(zeta_x, 2) - 2 * pow(zeta_x, 3))
                / pow(1 + 4 * zeta_x + 4 * pow(zeta_x, 2) - 4 * pow(zeta_x, 3) - pow(zeta_x, 4), 2);
    }
    inline double eta_func(double rho, double d_BH){return rho * PI * pow(d_BH, 3.0) / 6.0;}
    double zeta_x_func(double rho,
                    const std::vector<double>& x,
                    const std::vector<std::vector<double>>& d_BH);
    double dzetax_drho_func(const std::vector<double>& x,
                    const std::vector<std::vector<double>>& d_BH);

    double zeta_eff_func(double rho,
                    const std::vector<double>& x,
                    const std::vector<std::vector<double>>& d_BH,
                    double lambdaij);

    double dzeta_eff_drho_func(double rho,
                    const std::vector<double>& x,
                    const std::vector<std::vector<double>>& d_BH,
                    double lambdaij);

    std::vector<double> f_corr(double alpha);
    
};

namespace mie_rdf_constants{

// Gauss Legendre points for computing barker henderson diamenter (see: ThermoPack, SAFT-VR-Mie docs)
constexpr double gl_x[10] = {-0.973906528517171720078, -0.8650633666889845107321,
                            -0.6794095682990244062343, -0.4333953941292471907993,
                            -0.1488743389816312108848,  0.1488743389816312108848,
                            0.4333953941292471907993,  0.6794095682990244062343,
                            0.8650633666889845107321,  0.973906528517171720078};
constexpr double gl_w[10] = {0.0666713443086881375936, 0.149451349150580593146,
                            0.219086362515982043996, 0.2692667193099963550912,
                            0.2955242247147528701739, 0.295524224714752870174,
                            0.269266719309996355091, 0.2190863625159820439955,
                            0.1494513491505805931458, 0.0666713443086881375936};

constexpr double C_coeff_matr[4][4] // See Eq. A18 of J. Chem. Phys. 139, 154504 (2013); https://doi.org/10.1063/1.4819786
    {
        {0.81096, 1.7888, -37.578, 92.284},
        {1.0205, -19.341, 151.26, -463.50},
        {-1.9057, 22.845, -228.14, 973.92},
        {1.0885, -6.1962, 106.98, -677.64}
    };

constexpr double phi[7][3] // J. Chem. Phys. 139, 154504 (2013); https://doi.org/10.1063/1.4819786, Table II
        {
            {7.5365557, -359.44 , 1550.9 },
            {-37.60463, 1825.6  , -5070.1},
            {71.745953, -3168.0 , 6534.6 },
            {-46.83552, 1884.2  , -3288.7},
            {-2.467982, -0.82376, -2.7171},
            {-0.50272 , -3.1935 , 2.0883 },
            {8.0956883, 3.7090  , 0.0}
        };

}