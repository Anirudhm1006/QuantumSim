#include <stdexcept>

#include "IQuantumObject.hpp"
#include "HydrogenModel.hpp"
#include "WavePacket.hpp"
#include "SpinSystem.hpp"
#include "Laser.hpp"
#include "Entanglement.hpp"

std::unique_ptr<IQuantumObject> IQuantumObject::from_json(const nlohmann::json& j) {
    std::string type = j.at("type").get<std::string>();

    if (type == "HydrogenModel") {
        int n = j.at("n").get<int>();
        int l = j.at("l").get<int>();
        int m = j.at("m").get<int>();
        int Z = j.value("Z", 1);
        return std::make_unique<HydrogenModel>(n, l, m, Z);
    }

    if (type == "WavePacket") {
        double x0 = j.at("x0").get<double>();
        double v = j.at("v").get<double>();
        double sigma = j.at("sigma").get<double>();
        double k0 = j.at("k0").get<double>();
        double omega = j.at("omega").get<double>();
        return std::make_unique<WavePacket>(x0, v, sigma, k0, omega);
    }

    if (type == "SpinSystem") {
        double theta = j.at("theta").get<double>();
        double phi = j.at("phi").get<double>();
        return std::make_unique<SpinSystem>(theta, phi);
    }

    if (type == "LaserSystem") {
        int scheme = j.value("scheme", 2);
        auto laser = std::make_unique<LaserSystem>(static_cast<LaserLevelScheme>(scheme));
        return laser;
    }

    if (type == "EntanglementSystem") {
        TwoQubitState state;
        state.amp_00 = {j.at("amp_00_re").get<double>(), j.at("amp_00_im").get<double>()};
        state.amp_01 = {j.at("amp_01_re").get<double>(), j.at("amp_01_im").get<double>()};
        state.amp_10 = {j.at("amp_10_re").get<double>(), j.at("amp_10_im").get<double>()};
        state.amp_11 = {j.at("amp_11_re").get<double>(), j.at("amp_11_im").get<double>()};
        return std::make_unique<EntanglementSystem>(state);
    }

    throw std::runtime_error("Unknown IQuantumObject type: " + type);
}
