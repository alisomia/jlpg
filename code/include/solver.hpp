#ifndef __SOLVER_HPP__
#define __SOLVER_HPP__

#ifndef __PROBLEM_HPP__
#include "problem.hpp"
#endif

#include <cmath>
#include <time.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

const double ninf = -std::numeric_limits<double>::infinity();

enum StepSizeRule { Constant, Armijo, Nonmonotone, Classical };

struct Options{
	int maxiter;
	Real ftol, gtol;
	Real alpha, eta;
	Real rho;
	StepSizeRule rule;

	bool LineSearchEnabled, BBMethodEnabled;
	// bool ContinuationEnabled;

	// for BB method:
	int M;
	
	

	Options(int maxiter, Real ftol, Real gtol, Real alpha, Real eta): maxiter(maxiter), ftol(ftol), gtol(gtol), alpha(alpha), eta(eta) {};

	void setConstant() {
		LineSearchEnabled = false;
		BBMethodEnabled = false;
		rule = Constant;
	}

	void setArmijo(Real rho_) {
		LineSearchEnabled = true;
		BBMethodEnabled = true;
		rule = Armijo;
		rho = rho_;
	}

	void setNonmonotone(Real rho_, int M_) {
		LineSearchEnabled = true;
		BBMethodEnabled = true;
		rule = Nonmonotone;
		rho = rho_;
		M = M_;
	}

	void setClassical() {
		LineSearchEnabled = true;
		BBMethodEnabled = true;
		rule = Classical;
	}
	
};

struct Outputs{
	int iter;
	bool Flag;
	Real cputime; 
	Real F_cur, f_cur, nrmG;
};

template <typename T>
T pgm(Problem<T> p, T x, Options opts, Outputs& output){
#if VERBOSED
    cout << "## Proximal Gradient Method " << endl;
	cout << "with ftol = " << opts.ftol << "  gtol = " << opts.gtol << endl;
#endif
	Real F_prev, f_prev;	// F = f + h
	T x_prev, g_prev;
	Real F_cur = p.value(x);
    Real f_cur = p.f(x);
	T g_cur = p.gradf(x);
	Real alpha = opts.alpha;
	T dx, dg;
	Real dxg;

    int iter, nls;
	Real tmp, nrmG;
	Real f_hist_max;
	clock_t  Tstart, Tend;
	Tstart = clock();
	bool lsFlag;
	vector<Real> f_hist;
	output.Flag = false;

	if (opts.rule == Nonmonotone) {
		f_hist = vector<Real>(opts.M, ninf);
	}

    for( iter = 0; iter < opts.maxiter; ++iter ){
		if (opts.rule == Nonmonotone) {
			f_hist[iter % opts.M] = f_cur;
			f_hist_max = *max_element(f_hist.begin(), f_hist.end());
		}

		F_prev = F_cur;
		f_prev = f_cur;
		g_prev = g_cur;
		x_prev = x;

        x = p.proxh(x_prev - alpha * g_prev, alpha * p.mu);
		if (opts.LineSearchEnabled) {
			nls = 0;
			lsFlag = true;
			while (true) {
				tmp = p.f(x);

				switch (opts.rule) {
					case Armijo:
					if (tmp <= f_prev + opts.rho * (g_prev.array()*(x - x_prev).array()).sum()) lsFlag = false;
					break;

					case Nonmonotone:
					if (tmp <= f_hist_max + opts.rho * (g_prev.array()*(x - x_prev).array()).sum()) lsFlag = false;
					break;

					case Classical:
					if (tmp <= f_prev + (g_prev.array()*(x - x_prev).array()).sum() + .5 / alpha * (x - x_prev).squaredNorm()) lsFlag = false;
					break;

					case Constant:
					break;
				}

				if (nls == 20) lsFlag = false;
				if (!lsFlag) break;
				
				alpha = opts.eta * alpha; nls = nls + 1;
				x = p.proxh(x_prev - alpha * g_prev, alpha * p.mu);
			}

			f_cur = tmp;
			F_cur = p.value(x);
		}
		else {
			f_cur = p.f(x);
			F_cur = p.value(x);
		}
		
		g_cur = p.gradf(x);
		nrmG = (x - p.proxh(x - g_cur, p.mu)).norm();

#if VERBOSED
    cout << "In iteration " << iter << ", function value = "<< F_cur << ", optimality = " << nrmG << endl;
#endif

		if (opts.LineSearchEnabled && opts.BBMethodEnabled) {
			dx = x - x_prev;
			dg = g_cur - g_prev;
			dxg = abs(dx.array()*dg.array()).sum();

			if (dxg > 0) {
				if (iter & 1) {
					alpha = dxg / dg.squaredNorm();
				}
				else {
					alpha = dx.squaredNorm() / dxg;
				} 
			}
			alpha = min(max(alpha, 1e-4), 1e4);
		} 
		else {
			alpha = opts.alpha;
		}

        if (abs(F_cur - F_prev) < opts.ftol || nrmG < opts.gtol){
			output.Flag = true;
			break;
		} 
    }
	Tend = clock();
	Real during = (double)(Tend - Tstart)/CLOCKS_PER_SEC;
#if VERBOSED
    cout << "Problem Solved within " << iter+1 << " Iteration(s)." << endl;
	cout << "function value = " << F_cur << "; optimality = " << nrmG << "." << endl;
	cout << "Used Time:" << during << "(s)" << endl;
#endif
	output.iter = iter;
	output.cputime = during;
	output.F_cur = F_cur;
	output.f_cur = f_cur;
	output.nrmG = nrmG; 
    return x;
};


#endif
