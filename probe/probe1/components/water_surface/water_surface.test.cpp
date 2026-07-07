// Copyright 2025 Travis West
// Tests param_registry integration with WaterSurface (no GL context needed).
#include "param_registry.hpp"
#include "water_surface.hpp"
#include <gtest/gtest.h>
#include <string>

TEST(WaterSurfaceInputs, ToJsonContainsCellSize) {
    WaterSurface ws;
    std::string json = to_json(ws);
    EXPECT_NE(json.find("\"cell_size\""), std::string::npos);
}

TEST(WaterSurfaceInputs, ToJsonContainsFoamThreshold) {
    WaterSurface ws;
    std::string json = to_json(ws);
    EXPECT_NE(json.find("\"foam_threshold\""), std::string::npos);
}

TEST(WaterSurfaceInputs, ToJsonContainsSunIntensity) {
    WaterSurface ws;
    std::string json = to_json(ws);
    EXPECT_NE(json.find("\"sun_intensity\""), std::string::npos);
}

TEST(WaterSurfaceInputs, DefaultInputsMatchExpected) {
    WaterSurface ws;
    EXPECT_NEAR(ws.endpoints.cell_size.get(),      1.0f,  1e-5f);
    EXPECT_NEAR(ws.endpoints.foam_threshold.get(), 0.55f, 1e-5f);
    EXPECT_NEAR(ws.endpoints.sun_intensity.get(),  1.2f,  1e-5f);
}
