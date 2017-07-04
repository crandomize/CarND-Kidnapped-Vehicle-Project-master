/*
 * particle_filter.cpp
 *
 *  Created on: Dec 12, 2016
 *      Author: Tiffany Huang
 */

#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <math.h> 
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>

#include "particle_filter.h"

using namespace std;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
	// TODO: Set the number of particles. Initialize all particles to first position (based on estimates of 
	//   x, y, theta and their uncertainties from GPS) and all weights to 1. 
	// Add random Gaussian noise to each particle.
	// NOTE: Consult particle_filter.h for more information about this method (and others in this file).
	num_particles = 100;


	default_random_engine gen;

	normal_distribution<double> dist_x(x, std[0]);
	normal_distribution<double> dist_y(y, std[1]);
	normal_distribution<double> dist_psi(theta, std[2]);

	for (int i=0; i<num_particles; i++) {
		Particle p;
		p.x = dist_x(gen);
		p.y = dist_y(gen);
		p.theta = dist_psi(gen);
		p.weight = 1;

		particles.push_back(p);

		//cout << "particle" << p.x << endl;

		weights.push_back(1);
	}

	is_initialized = true;
}

void ParticleFilter::prediction(double delta_t, double std_pos[], double velocity, double yaw_rate) {
	// TODO: Add measurements to each particle and add random Gaussian noise.
	// NOTE: When adding noise you may find std::normal_distribution and std::default_random_engine useful.
	//  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
	//  http://www.cplusplus.com/reference/random/default_random_engine/

	double xf = 0;
	double yf = 0;
	double thetaf = 0;

	default_random_engine gen;

	normal_distribution<double> dist_x(0, std_pos[0]);
	normal_distribution<double> dist_y(0, std_pos[1]);
	normal_distribution<double> dist_psi(0, std_pos[2]);


	for (int i=0; i<num_particles; i++) {
		Particle p = particles[i];
		if(yaw_rate != 0) {
			xf = p.x + velocity*(1/yaw_rate)*( sin(p.theta + yaw_rate*delta_t) - sin(p.theta) );
			yf = p.y + velocity*(1/yaw_rate)*( cos(p.theta) - cos(p.theta + yaw_rate*delta_t) );
			thetaf = p.theta + yaw_rate*delta_t;
		} else {
			xf = p.x + velocity*delta_t*cos(p.theta);
			yf = p.y + velocity*delta_t*sin(p.theta);
			thetaf = p.theta;
		}



		particles[i].x = xf + dist_x(gen);
		particles[i].y = yf + dist_y(gen);
		particles[i].theta = thetaf + dist_psi(gen);

	}
	
}

void ParticleFilter::dataAssociation(std::vector<LandmarkObs> predicted, std::vector<LandmarkObs>& observations) {
	// TODO: Find the predicted measurement that is closest to each observed measurement and assign the 
	//   observed measurement to this particular landmark.
	// NOTE: this method will NOT be called by the grading code. But you will probably find it useful to 
	//   implement this method and use it as a helper during the updateWeights phase.

	int num_observations;
	num_observations = observations.size();

	int num_maplandmarks;
	num_maplandmarks = predicted.size();

	for (int i=0; i<num_observations; i++) {
		double x = observations[i].x;
		double y = observations[i].y;

		int id;
		double min_distance = 50;

		for (int j=0; j<num_maplandmarks; j++) {
			double xm = predicted[j].x;
			double ym = predicted[j].y;
			double d = dist(x, y, xm, ym);

			if(d < min_distance) {
				id = predicted[j].id;
				min_distance = d;
				//cout <<"found " << d << endl;
			}
		}

		observations[i].id = id;
	}

}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
		std::vector<LandmarkObs> observations, Map map_landmarks) {
	// TODO: Update the weights of each particle using a mult-variate Gaussian distribution. You can read
	//   more about this distribution here: https://en.wikipedia.org/wiki/Multivariate_normal_distribution
	// NOTE: The observations are given in the VEHICLE'S coordinate system. Your particles are located
	//   according to the MAP'S coordinate system. You will need to transform between the two systems.
	//   Keep in mind that this transformation requires both rotation AND translation (but no scaling).
	//   The following is a good resource for the theory:
	//   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
	//   and the following is a good resource for the actual equation to implement (look at equation 
	//   3.33
	//   http://planning.cs.uiuc.edu/node99.html


	double sigmax = std_landmark[0];
	double sigmay = std_landmark[1];

	// Convert map_landmarks to predicted observations

	int num_maplandmarks = map_landmarks.landmark_list.size();


	for (int i=0; i<num_particles; i++) {
		vector<LandmarkObs> absoluteobservations;
		int num_obs = observations.size();

		Particle p = particles[i];

		vector<LandmarkObs> predictedobservations;

		int counterId = 1;  //recalculate ids so that we can index later.
		for (int k=0; k<num_maplandmarks; k++) {
			LandmarkObs l;
			l.x = map_landmarks.landmark_list[k].x_f;
			l.y = map_landmarks.landmark_list[k].y_f;

			if(dist(l.x,l.y,p.x,p.y) <= sensor_range) {
				l.id = counterId; //map_landmarks.landmark_list[k].id_i;
				counterId++;
				predictedobservations.push_back(l);
			}
		}


		for (int j=0; j<num_obs; j++) {
			double xr = observations[j].x;
			double yr = observations[j].y;

			LandmarkObs meas;

			meas.x = xr*cos(p.theta) - yr*sin(p.theta) + p.x;
			meas.y = xr*sin(p.theta) + yr*cos(p.theta) + p.y;

			absoluteobservations.push_back(meas);
		}
		dataAssociation(predictedobservations, absoluteobservations);

		std::vector<double> pweights;
		double w = 1;
		// Calculate the weight.
		for (int j=0; j<num_obs; j++) {
			
			
			double x = absoluteobservations[j].x;
			double y = absoluteobservations[j].y;
			int id = absoluteobservations[j].id;

			if(id>0) {

				double xm = predictedobservations[id-1].x;
				double ym = predictedobservations[id-1].y;
				
				double p = (1/(2*M_PI*sigmax*sigmay)) * exp(-(  (x-xm)*(x-xm)/(2*sigmax*sigmax) + (y-ym)*(y-ym)/(2*sigmay*sigmay) ));
				w = w*p;
			}
		}
		weights[i] = w;
	}
}

void ParticleFilter::resample() {
	// TODO: Resample particles with replacement with probability proportional to their weight. 
	// NOTE: You may find std::discrete_distribution helpful here.
	//   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution

	// Setup the random bits
    std::random_device rd;
    std::mt19937 gen(rd());	

	std::discrete_distribution<> d(weights.begin(), weights.end());

	std::vector<Particle> nparticles;
	std::vector<double> nweights;


	for (int i=0; i<num_particles; i++) {

		//Get random particle

		int rnd = d(gen);
		nparticles.push_back(particles[rnd]);
		nweights.push_back(1);
	}	

	particles = nparticles;
	weights = nweights;

}

Particle ParticleFilter::SetAssociations(Particle particle, std::vector<int> associations, std::vector<double> sense_x, std::vector<double> sense_y)
{
	//particle: the particle to assign each listed association, and association's (x,y) world coordinates mapping to
	// associations: The landmark id that goes along with each listed association
	// sense_x: the associations x mapping already converted to world coordinates
	// sense_y: the associations y mapping already converted to world coordinates

	//Clear the previous associations
	particle.associations.clear();
	particle.sense_x.clear();
	particle.sense_y.clear();

	particle.associations= associations;
 	particle.sense_x = sense_x;
 	particle.sense_y = sense_y;

 	return particle;
}

string ParticleFilter::getAssociations(Particle best)
{
	vector<int> v = best.associations;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<int>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseX(Particle best)
{
	vector<double> v = best.sense_x;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseY(Particle best)
{
	vector<double> v = best.sense_y;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
