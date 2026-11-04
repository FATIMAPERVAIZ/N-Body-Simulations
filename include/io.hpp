#pragma once
#include <fstream>
#include <string>
#include <vector>

#include "body.hpp"

// Saves id,x,y,vx,vy,m with full double precision, so two runs can be compared
// exactly. Creates the folder if needed.
bool save_state_csv(const std::string& path, const std::vector<Body>& bodies, std::string& error);

// Snapshots: one file with rows step,id,x,y,vx,vy,m
bool open_snapshot_file(const std::string& path, std::ofstream& out, std::string& error);
void append_snapshot(std::ofstream& out, int step, const std::vector<Body>& bodies);
