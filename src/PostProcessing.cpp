#include <RcppArmadillo.h>
#include <splines2Armadillo.h>
#include <cmath>
#include "CalculateLikelihood.H"
//' Calculates the Pointwise credible interval for the mean
//'
//' @name GetMeanCI_Pw
//' @param dir String containing the directory where the MCMC files are located
//' @param n_files Int containing the number of files per parameter
//' @param time Vector containing time points of interest
//' @param k Int containing the cluster group of which you want to get the credible interval for
//' @return CI list containing the 97.5th , 50th, and 2.5th pointwise credible functions
//' @export
// [[Rcpp::export]]
Rcpp::List GetMeanCI_PW(const std::string dir,
                        const int n_files,
                        const arma::vec time,
                        const int k){
  arma::cube nu_i;
  nu_i.load(dir + "Nu0.txt");
  arma::cube nu_samp = arma::zeros(nu_i.n_rows, nu_i.n_cols, nu_i.n_slices * n_files);
  nu_samp.subcube(0, 0, 0, nu_i.n_rows-1, nu_i.n_cols-1, nu_i.n_slices-1) = nu_i;
  for(int i = 1; i < n_files; i++){
    nu_i.load(dir + "Nu" + std::to_string(i) +".txt");
    nu_samp.subcube(0, 0,  nu_i.n_slices*i, nu_i.n_rows-1, nu_i.n_cols-1,
                    (nu_i.n_slices)*(i+1) - 1) = nu_i;
  }
  splines2::BSpline bspline;
  bspline = splines2::BSpline(time, nu_i.n_cols);
  // Get Basis matrix (100 x 8)
  arma::mat bspline_mat {bspline.basis(true)};
  // Make B_obs
  arma::mat B = bspline_mat;

  arma::mat f_samp = arma::zeros(nu_samp.n_slices, time.n_elem);
  for(int i = 0; i < nu_samp.n_slices; i++){
    f_samp.row(i) = (B * nu_samp.slice(i).row(k-1).t()).t();
  }

  // Initialize placeholders
  arma::vec CI_975 = arma::zeros(time.n_elem);
  arma::vec CI_50 = arma::zeros(time.n_elem);
  arma::vec CI_025 = arma::zeros(time.n_elem);

  arma::vec p = {0.025, 0.5, 0.975};
  arma::vec q = arma::zeros(3);

  for(int i = 0; i < time.n_elem; i++){
    q = arma::quantile(f_samp.col(i), p);
    CI_025(i) = q(0);
    CI_50(i) = q(1);
    CI_975(i) = q(2);
  }
  Rcpp::List CI =  Rcpp::List::create(Rcpp::Named("CI_975", CI_975),
                                      Rcpp::Named("CI_50", CI_50),
                                      Rcpp::Named("CI_025", CI_025));
  return(CI);
}

//' Calculates the simultaneous credible interval for the mean
//'
//' @name GetMeanCI_S
//' @param dir String containing the directory where the MCMC files are located
//' @param n_files Int containing the number of files per parameter
//' @param time Vector containing time points of interest
//' @param k Int containing the cluster group of which you want to get the credible interval for
//' @return CI list containing the 97.5th , 50th, and 2.5th simultaneous credible functions
//' @export
// [[Rcpp::export]]
Rcpp::List GetMeanCI_S(const std::string dir,
                       const int n_files,
                       const arma::vec time,
                       const int k){
  arma::cube nu_i;
  nu_i.load(dir + "Nu0.txt");
  arma::cube nu_samp = arma::zeros(nu_i.n_rows, nu_i.n_cols, nu_i.n_slices * n_files);
  nu_samp.subcube(0, 0, 0, nu_i.n_rows-1, nu_i.n_cols-1, nu_i.n_slices-1) = nu_i;
  for(int i = 1; i < n_files; i++){
    nu_i.load(dir + "Nu" + std::to_string(i) +".txt");
    nu_samp.subcube(0, 0,  nu_i.n_slices*i, nu_i.n_rows-1, nu_i.n_cols-1,
                    (nu_i.n_slices)*(i+1) - 1) = nu_i;
  }
  splines2::BSpline bspline;
  bspline = splines2::BSpline(time, nu_i.n_cols);
  // Get Basis matrix (100 x 8)
  arma::mat bspline_mat {bspline.basis(true)};
  // Make B_obs
  arma::mat B = bspline_mat;

  arma::mat f_samp = arma::zeros(nu_samp.n_slices, time.n_elem);
  for(int i = 0; i < nu_samp.n_slices; i++){
    f_samp.row(i) = (B * nu_samp.slice(i).row(k-1).t()).t();
  }
  arma::rowvec f_mean = arma::mean(f_samp, 0);
  arma::rowvec f_sd = arma::stddev(f_samp, 0, 0);

  arma::vec C = arma::zeros(nu_samp.n_slices);
  arma::vec ph1 = arma::zeros(time.n_elem);
  for(int i = 0; i < nu_samp.n_slices; i++){
    for(int j = 0; j < time.n_elem; j++){
      ph1(j) = std::abs((f_samp(i,j) - f_mean(j)) / f_sd(j));
    }
    C(i) = arma::max(ph1);
  }

  // Initialize placeholders
  arma::vec CI_975 = arma::zeros(time.n_elem);
  arma::vec CI_50 = arma::zeros(time.n_elem);
  arma::vec CI_025 = arma::zeros(time.n_elem);

  arma::vec p = {0.95};
  arma::vec q = arma::zeros(1);
  q = arma::quantile(C, p);

  for(int i = 0; i < time.n_elem; i++){
    CI_025(i) = f_mean(i) - q(0) * f_sd(i);
    CI_50(i) = f_mean(i);
    CI_975(i) =  f_mean(i) + q(0) * f_sd(i);
  }
  Rcpp::List CI =  Rcpp::List::create(Rcpp::Named("CI_975", CI_975),
                                      Rcpp::Named("CI_50", CI_50),
                                      Rcpp::Named("CI_025", CI_025));
  return(CI);
}

//' Calculates the Pointwise credible interval for the covariance function between two clusters
//'
//' @name GetCovCI_Pw
//' @param dir String containing the directory where the MCMC files are located
//' @param n_files Int containing the number of files per parameter
//' @param n_MCMC Int containing the number of saved MCMC iterations per file
//' @param time1 Vector containing time points of interest for first cluster
//' @param time2 Vector containing time points of interest for second cluster
//' @param l Int containing the 1st cluster group of which you want to get the credible interval for
//' @param m Int containing the 2nd cluster group of which you want to get the credible interval for
//' @return CI list containing the 97.5th , 50th, and 2.5th pointwise credible functions
//' @export
// [[Rcpp::export]]
Rcpp::List GetCovCI_Pw(const std::string dir,
                       const int n_files,
                       const int n_MCMC,
                       const arma::vec time1,
                       const arma::vec time2,
                       const int l,
                       const int m){
  // Get Phi Paramters
  arma::field<arma::cube> phi_i;
  phi_i.load(dir + "Phi0.txt");
  arma::field<arma::cube> phi_samp(n_MCMC * n_files, 1);
  for(int i = 0; i < n_MCMC, i++){
    phi_samp(i,0) = phi_i(i,0);
  }

  for(int i = 1; i < n_files; i++){
    phi_i.load(dir + "Phi" + std::to_string(i) +".txt");
    for(int j = 0; j < n_MCMC, j++){
      phi_samp((i * n_MCMC) + j, 0) = phi_i(i,0);
    }
  }

  // Make spline basis 1
  splines2::BSpline bspline1;
  bspline1 = splines2::BSpline(time1, Phi.n_cols);
  // Get Basis matrix (time1 x Phi.n_cols)
  arma::mat bspline_mat1{bspline1.basis(true)};
  // Make B_obs
  arma::mat B1 = bspline_mat1;

  // Make spline basis 2
  splines2::BSpline bspline2;
  bspline2 = splines2::BSpline(time2, Phi.n_cols);
  // Get Basis matrix (time2 x Phi.n_cols)
  arma::mat bspline_mat2{bspline2.basis(true)};
  // Make B_obs
  arma::mat B2 = bspline_mat2;

  arma::cube cov_samp = arma::zeros(time1.n_elem, time2.n_elem, n_MCMC * n_files);
  for(int i = 0; i < n_MCMC * n_files, i++){
    for(int j = 0; j < phi_samp.n_slices; j++){
      cov_samp.slice(i) = cov_samp.slice(i) + (B1 * phi_samp.slice(j).row(l) *
        (B2 * phi_samp.slice(j).row(m)).t());
    }
  }

  // Initialize placeholders
  arma::vec CI_975 = arma::zeros(time1.n_elem, time2.n_elem);
  arma::vec CI_50 = arma::zeros(time1.n_elem, time2.n_elem);
  arma::vec CI_025 = arma::zeros(time2.n_elem, time2.n_elem);

  arma::vec p = {0.025, 0.5, 0.975};
  arma::vec q = arma::zeros(3);

  for(int i = 0; i < time1.n_elem; i++){
    for(int j = 0; j < time2.n_elem; j++){
      q = arma::quantile(cov_samp.col(i), p);
      CI_025(i,j) = q(0);
      CI_50(i,j) = q(1);
      CI_975(i,j) = q(2);
    }
  }

  Rcpp::List CI =  Rcpp::List::create(Rcpp::Named("CI_975", CI_975),
                                      Rcpp::Named("CI_50", CI_50),
                                      Rcpp::Named("CI_025", CI_025));
  return(CI);
}

//' Calculates the Simultaneous credible interval for the covariance function between two clusters
//'
//' @name GetCovCI_S
//' @param dir String containing the directory where the MCMC files are located
//' @param n_files Int containing the number of files per parameter
//' @param n_MCMC Int containing the number of saved MCMC iterations per file
//' @param time1 Vector containing time points of interest for first cluster
//' @param time2 Vector containing time points of interest for second cluster
//' @param l Int containing the 1st cluster group of which you want to get the credible interval for
//' @param m Int containing the 2nd cluster group of which you want to get the credible interval for
//' @return CI list containing the 97.5th , 50th, and 2.5th simultaneous credible functions
//' @export
// [[Rcpp::export]]
Rcpp::List GetCovCI_S(const std::string dir,
                      const int n_files,
                      const int n_MCMC,
                      const arma::vec time1,
                      const arma::vec time2,
                      const int l,
                      const int m){

  // Get Phi Paramters
  arma::field<arma::cube> phi_i;
  phi_i.load(dir + "Phi0.txt");
  arma::field<arma::cube> phi_samp(n_MCMC * n_files, 1);
  for(int i = 0; i < n_MCMC, i++){
    phi_samp(i,0) = phi_i(i,0);
  }

  for(int i = 1; i < n_files; i++){
    phi_i.load(dir + "Phi" + std::to_string(i) +".txt");
    for(int j = 0; j < n_MCMC, j++){
      phi_samp((i * n_MCMC) + j, 0) = phi_i(i,0);
    }
  }

  // Make spline basis 1
  splines2::BSpline bspline1;
  bspline1 = splines2::BSpline(time1, Phi.n_cols);
  // Get Basis matrix (time1 x Phi.n_cols)
  arma::mat bspline_mat1{bspline1.basis(true)};
  // Make B_obs
  arma::mat B1 = bspline_mat1;

  // Make spline basis 2
  splines2::BSpline bspline2;
  bspline2 = splines2::BSpline(time2, Phi.n_cols);
  // Get Basis matrix (time2 x Phi.n_cols)
  arma::mat bspline_mat2{bspline2.basis(true)};
  // Make B_obs
  arma::mat B2 = bspline_mat2;

  arma::cube cov_samp = arma::zeros(time1.n_elem, time2.n_elem, n_MCMC * n_files);
  for(int i = 0; i < n_MCMC * n_files, i++){
    for(int j = 0; j < phi_samp.n_slices; j++){
      cov_samp.slice(i) = cov_samp.slice(i) + (B1 * phi_samp.slice(j).row(l) *
        (B2 * phi_samp.slice(j).row(m)).t());
    }
  }

  arma::mat cov_mean = arma::mean(cov_samp, 2);
  arma::mat cov_sd = arma::zeros(time1.n_elem, time2.n_elem);
  for(int i = 0; i < time1.n_elem; i++){
    for(int j = 0; j < time2.n_elem; j++){
      cov_sd(i,j) = arma::stddev(cov_samp.row(i).col(j));
    }
  }

  arma::vec C = arma::zeros(cov_samp.n_slices);
  arma::vec ph1 = arma::zeros(time1.n_elem, time2.n_elem);
  for(int i = 0; i < nu_samp.n_slices; i++){
    for(int j = 0; j < time1.n_elem; j++){
      for(int k = 0; k < time2.n_elem; k++){
        ph1(j,k) = std::abs((cov_samp(j,k,i) - cov_mean(j,k)) / f_sd(j,k));
      }
    }
    C(i) = arma::ph1.max();
  }

  arma::vec p = {0.95};
  arma::vec q = arma::zeros(1);
  q = arma::quantile(C, p);

  // Initialize placeholders
  arma::vec CI_975 = arma::zeros(time1.n_elem, time2.n_elem);
  arma::vec CI_50 = arma::zeros(time1.n_elem, time2.n_elem);
  arma::vec CI_025 = arma::zeros(time2.n_elem, time2.n_elem);

  for(int i = 0; i < time1.n_elem; i++){
    for(int j = 0; j < time2.n_elem; j++){
      CI_025(i,j) = f_mean(i,j) - q(0) * f_sd(i,j);
      CI_50(i,j) = f_mean(i,j);
      CI_975(i,j) =  f_mean(i,j) + q(0) * f_sd(i,j);;
    }
  }

  Rcpp::List CI =  Rcpp::List::create(Rcpp::Named("CI_975", CI_975),
                                      Rcpp::Named("CI_50", CI_50),
                                      Rcpp::Named("CI_025", CI_025));
  return(CI);
}

//' Calculates the credible interval for sigma squared
//'
//' @name GetSigmaCI
//' @param dir String containing the directory where the MCMC files are located
//' @return CI list containing the 97.5th , 50th, and 2.5th pointwise credible values
//' @export
// [[Rcpp::export]]
Rcpp::List GetSigamCI(const std::string dir,
                      const int n_files){
  arma::vec sigma_i;
  sigma_i.load(dir + "Sigma0.txt");
  arma::vec sigma_samp = arma::zeros(sigma_i.n_elem * n_files);
  sigma_samp.subvec(0, sigma_i.n_elem - 1) = sigma_i;
  for(int i = 1; i < n_files; i++){
    sigma_i.load(dir + "Sigma" + std::to_string(i) +".txt");
    sigma_samp.subvec(sigma_i.n_elem *i, (sigma_i.n_elem *(i + 1)) - 1) = sigma_i;
  }

  arma::vec p = {0.025, 0.5, 0.975};
  arma::vec q = arma::zeros(3);
  q = arma::quantile(sigma_samp, p);

  double CI_975 = q(0);
  double CI_50 = q(1);
  double CI_025 = q(2);

  Rcpp::List CI =  Rcpp::List::create(Rcpp::Named("CI_975", CI_975),
                                      Rcpp::Named("CI_50", CI_50),
                                      Rcpp::Named("CI_025", CI_025));

  return(CI);
}

//' Calculates the credible interval for membership parameters Z
//'
//' @name GetZCI
//' @param dir String containing the directory where the MCMC files are located
//' @param n_files Int containing the number of files per parameter
//' @return CI list containing the 97.5th , 50th, and 2.5th credible values
//' @export
// [[Rcpp::export]]
Rcpp::List GetZCI(const std::string dir,
                  const int n_files){
  arma::cube Z_i;
  Z_i.load(dir + "Z0.txt");
  arma::vec Z_samp = arma::zeros(Z_i.n_slices * n_files);
  Z_samp.subcube(0, 0, 0, Z_i.n_rows-1, Z_i.n_cols-1, Z_i.n_slices-1) = Z_i;
  for(int i = 1; i < n_files; i++){
    Z_i.load(dir + "Z" + std::to_string(i) +".txt");
    Z_samp.subcube(0, 0,  Z_i.n_slices*i, Z_i.n_rows-1, Z_i.n_cols-1, (Z_i.n_slices)*(i+1) - 1) = Z_i;
  }

  arma::vec p = {0.025, 0.5, 0.975};
  arma::vec q = arma::zeros(3);

  arma::mat CI_975 = arma::zeros(Z_i.n_rows, Z_i.n_cols);
  arma::mat CI_50 = arma::zeros(Z_i.n_rows, Z_i.n_cols);
  arma::mat CI_025 = arma::zeros(Z_i.n_rows, Z_i.n_cols);

  for(int i = 0; i < Z_i.n_rows; i++){
    for(int j = 0; j < Z_i.n_cols; j++){
      q = arma::quantile(Z_samp.row(i).col(j), p);
      CI_975(i,j) = q(0);
      CI_50(i,j) = q(1);
      CI_025(i,j) = q(2);
    }
  }

  Rcpp::List CI =  Rcpp::List::create(Rcpp::Named("CI_975", CI_975),
                                      Rcpp::Named("CI_50", CI_50),
                                      Rcpp::Named("CI_025", CI_025));

  return(CI);
}

//' Calculates the AIC of a model
//'
//' @name Model_AIC
//' @param dir String containing the directory where the MCMC files are located
//' @param n_files Int containing the number of files per parameter
//' @param n_MCMC Int containing the number of saved MCMC iterations per file
//' @param n_obs Int containing the number of functions observed
//' @param time Field of vectors containing time points at which the function was observed
//' @param Y Field of vectors containing observed values of the function
//' @returns AIC Double containing AIC value
//' @export
// [[Rcpp::export]]
double Model_DIC(const std::String dir,
                 const int n_files,
                 const int n_MCMC,
                 const int n_obs,
                 const arma::field<arma::vec> time,
                 const arma::field<arma::vec> Y){
  // Get Nu parameters
  arma::cube nu_i;
  nu_i.load(dir + "Nu0.txt");
  arma::cube nu_samp = arma::zeros(nu_i.n_rows, nu_i.n_cols, nu_i.n_slices * n_files);
  nu_samp.subcube(0, 0, 0, nu_i.n_rows-1, nu_i.n_cols-1, nu_i.n_slices-1) = nu_i;
  for(int i = 1; i < n_files; i++){
    nu_i.load(dir + "Nu" + std::to_string(i) +".txt");
    nu_samp.subcube(0, 0,  nu_i.n_slices*i, nu_i.n_rows-1, nu_i.n_cols-1,
                    (nu_i.n_slices)*(i+1) - 1) = nu_i;
  }

  // Get Phi parameters
  arma::field<arma::cube> phi_i;
  phi_i.load(dir + "Phi0.txt");
  arma::field<arma::cube> phi_samp(n_MCMC * n_files, 1);
  for(int i = 0; i < n_MCMC, i++){
    phi_samp(i,0) = phi_i(i,0);
  }

  for(int i = 1; i < n_files; i++){
    phi_i.load(dir + "Phi" + std::to_string(i) +".txt");
    for(int j = 0; j < n_MCMC, j++){
      phi_samp((i * n_MCMC) + j, 0) = phi_i(i,0);
    }
  }

  // Get Z parameters
  arma::cube Z_i;
  Z_i.load(dir + "Z0.txt");
  arma::vec Z_samp = arma::zeros(Z_i.n_slices * n_files);
  Z_samp.subcube(0, 0, 0, Z_i.n_rows-1, Z_i.n_cols-1, Z_i.n_slices-1) = Z_i;
  for(int i = 1; i < n_files; i++){
    Z_i.load(dir + "Z" + std::to_string(i) +".txt");
    Z_samp.subcube(0, 0,  Z_i.n_slices*i, Z_i.n_rows-1, Z_i.n_cols-1, (Z_i.n_slices)*(i+1) - 1) = Z_i;
  }

  // Get sigma parameters
  arma::vec sigma_i;
  sigma_i.load(dir + "Sigma0.txt");
  arma::vec sigma_samp = arma::zeros(sigma_i.n_elem * n_files);
  sigma_samp.subvec(0, sigma_i.n_elem - 1) = sigma_i;
  for(int i = 1; i < n_files; i++){
    sigma_i.load(dir + "Sigma" + std::to_string(i) +".txt");
    sigma_samp.subvec(sigma_i.n_elem *i, (sigma_i.n_elem *(i + 1)) - 1) = sigma_i;
  }

  // Get chi parameters
  arma::vec chi_i;
  chi_i.load(dir + "Chi0.txt");
  arma::vec chi_samp = arma::zeros(chi_i.n_slices * n_files);
  chi_samp.subcube(0, 0, 0, chi_i.n_rows-1, chi_i.n_cols-1, chi_i.n_slices-1) = chi_i;
  for(int i = 1; i < n_files; i++){
    chi_i.load(dir + "Chi" + std::to_string(i) +".txt");
    chi_samp.subcube(0, 0,  chi_i.n_slices*i, chi_i.n_rows-1, chi_i.n_cols-1,
                     (chi_i.n_slices)*(i+1) - 1) = chi_i;
  }

  // Make spline basis
  splines2::BSpline bspline2;
  for(int i = 0; i < n_funct; i++)
  {
    bspline2 = splines2::BSpline(time(i,0), Phi.n_cols);
    // Get Basis matrix (time2 x Phi.n_cols)
    arma::mat bspline_mat2{bspline2.basis(true)};
    // Make B_obs
    B_obs(i,0) = bspline_mat2;
  }

  double expected_log_f = 0;
  for(int i = 0; i < nu_samp.n_slices; i++){
    expected_log_f = expected_log_f + calcLikelihood(Y, B_obs, nu_samp.slice(i),
                                                     phi_samp(i,0), Z_samp.slice(i),
                                                     chi_samp.slice(i), sigma(i));
  }


}