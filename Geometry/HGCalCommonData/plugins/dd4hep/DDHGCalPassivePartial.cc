///////////////////////////////////////////////////////////////////////////////
// File: DDHGCalPassivePartial.cc
// Description: Geometry factory class for the passive part of a partial
//              silicon module
// Created by Sunanda Banerjee
///////////////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>
#include <sstream>

#include "DD4hep/DetFactoryHelper.h"
#include "DataFormats/Math/interface/angle_units.h"
#include "DetectorDescription/DDCMS/interface/DDPlugins.h"
#include "DetectorDescription/DDCMS/interface/DDutils.h"
#include "Geometry/HGCalCommonData/interface/HGCalWaferMask.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

//#define EDM_ML_DEBUG
using namespace angle_units::operators;

struct HGCalPassivePartial {
  HGCalPassivePartial() { throw cms::Exception("HGCalGeom") << "Wrong initialization to HGCalPassivePartial"; }
  HGCalPassivePartial(cms::DDParsingContext& ctxt, xml_h e) {
    cms::DDNamespace ns(ctxt, e, true);
    cms::DDAlgoArguments args(ctxt, e);
#ifdef EDM_ML_DEBUG
    edm::LogVerbatim("HGCalGeom") << "DDHGCalPassivePartial: Creating an instance";
#endif
    std::string parentName = args.parentName();
    std::string material = args.value<std::string>("ModuleMaterial");
    double thick = args.value<double>("ModuleThickness");
    double waferSize = args.value<double>("WaferSize");
    double waferSepar = args.value<double>("SensorSeparation");
#ifdef EDM_ML_DEBUG
    edm::LogVerbatim("HGCalGeom") << "DDHGCalPassivePartial: Module " << parentName << " made of " << material << " T "
                                  << thick << " Wafer 2r " << waferSize << " Half Separation " << waferSepar;
#endif
    std::vector<std::string> tags = args.value<std::vector<std::string>>("Tags");
    std::vector<int> partialTypes = args.value<std::vector<int>>("PartialTypes");
    std::vector<int> placementIndex = args.value<std::vector<int>>("PlacementIndex");
    std::vector<std::string> placementIndexTags = args.value<std::vector<std::string>>("PlacementIndexTags");
#ifdef EDM_ML_DEBUG
    edm::LogVerbatim("HGCalGeom") << "DDHGCalPassivePartial: " << tags.size() << " variations of wafer types";
    for (unsigned int k = 0; k < tags.size(); ++k) {
      for (unsigned int m = 0; m < placementIndex.size(); ++m) {
        edm::LogVerbatim("HGCalGeom") << "Type[" << k << "] " << tags[k] << " Partial " << partialTypes[k]
                                      << " Placement Index " << placementIndex[m] << " Tag " << placementIndexTags[m];
      }
    }
#endif
    std::vector<std::string> layerNames = args.value<std::vector<std::string>>("LayerNames");
    std::vector<std::string> materials = args.value<std::vector<std::string>>("LayerMaterials");
    std::vector<double> layerThick = args.value<std::vector<double>>("LayerThickness");
#ifdef EDM_ML_DEBUG
    edm::LogVerbatim("HGCalGeom") << "DDHGCalPassivePartial: " << layerNames.size() << " types of volumes";
    for (unsigned int i = 0; i < layerNames.size(); ++i)
      edm::LogVerbatim("HGCalGeom") << "Volume [" << i << "] " << layerNames[i] << " of thickness " << layerThick[i]
                                    << " filled with " << materials[i];
#endif
    std::vector<int> layerType = args.value<std::vector<int>>("LayerType");
#ifdef EDM_ML_DEBUG
    std::ostringstream st1;
    for (unsigned int i = 0; i < layerType.size(); ++i)
      st1 << " [" << i << "] " << layerType[i];
    edm::LogVerbatim("HGCalGeom") << "There are " << layerType.size() << " blocks" << st1.str();

    edm::LogVerbatim("HGCalGeom") << "==>> Executing DDHGCalPassivePartial...";
#endif

    static constexpr double tol = 0.00001;

    // Loop over all types
    for (unsigned int k = 0; k < tags.size(); ++k) {
      for (unsigned int m = 0; m < placementIndex.size(); ++m) {
        // First the mother
        std::string mother = parentName + placementIndexTags[m] + tags[k];
        std::vector<std::pair<double, double>> wxy =
            HGCalWaferMask::waferXY(partialTypes[k], placementIndex[m], (waferSize + waferSepar), 0.0, 0.0, 0.0);
        std::vector<double> xM, yM;
        for (unsigned int i = 0; i < (wxy.size() - 1); ++i) {
          xM.emplace_back(wxy[i].first);
          yM.emplace_back(wxy[i].second);
        }
        std::vector<double> zw = {-0.5 * thick, 0.5 * thick};
        std::vector<double> zx(2, 0), zy(2, 0), scale(2, 1.0);
        dd4hep::Solid solid = dd4hep::ExtrudedPolygon(xM, yM, zw, zx, zy, scale);
        ns.addSolidNS(ns.prepend(mother), solid);
        dd4hep::Material matter = ns.material(material);
        dd4hep::Volume glogM = dd4hep::Volume(solid.name(), solid, matter);
        ns.addVolumeNS(glogM);
#ifdef EDM_ML_DEBUG
        edm::LogVerbatim("HGCalGeom") << "DDHGCalPassivePartial: " << solid.name() << " extruded polygon made of "
                                      << matter.name() << " z|x|y|s (0) " << zw[0] << ":" << zx[0] << ":" << zy[0]
                                      << ":" << scale[0] << " z|x|y|s (1) " << zw[1] << ":" << zx[1] << ":" << zy[1]
                                      << ":" << scale[1] << " and " << xM.size() << " edges";
        for (unsigned int kk = 0; kk < xM.size(); ++kk)
          edm::LogVerbatim("HGCalGeom") << "[" << kk << "] " << xM[kk] << ":" << yM[kk];
#endif

        // Then the layers
        std::vector<dd4hep::Volume> glogs(materials.size());
        std::vector<int> copyNumber;
        copyNumber.resize(materials.size(), 1);
        double zi(-0.5 * thick), thickTot(0.0);
        for (unsigned int l = 0; l < layerType.size(); l++) {
          unsigned int i = layerType[l];
          if (copyNumber[i] == 1) {
            zw[0] = -0.5 * layerThick[i];
            zw[1] = 0.5 * layerThick[i];
            std::string layerName = mother + layerNames[i];
            solid = dd4hep::ExtrudedPolygon(xM, yM, zw, zx, zy, scale);
            ns.addSolidNS(ns.prepend(layerName), solid);
            matter = ns.material(materials[i]);
            glogs[i] = dd4hep::Volume(solid.name(), solid, matter);
            ns.addVolumeNS(glogs[i]);
#ifdef EDM_ML_DEBUG
            edm::LogVerbatim("HGCalGeom")
                << "DDHGCalPassivePartial: Layer " << i << ":" << l << ":" << solid.name()
                << " extruded polygon made of " << matter.name() << " z|x|y|s (0) " << zw[0] << ":" << zx[0] << ":"
                << zy[0] << ":" << scale[0] << " z|x|y|s (1) " << zw[1] << ":" << zx[1] << ":" << zy[1] << ":"
                << scale[1] << " and " << xM.size() << " edges";
            for (unsigned int kk = 0; kk < xM.size(); ++kk)
              edm::LogVerbatim("HGCalGeom") << "[" << kk << "] " << xM[kk] << ":" << yM[kk];
#endif
          }
          dd4hep::Position tran(0, 0, (zi + 0.5 * layerThick[i]));
          glogM.placeVolume(glogs[i], copyNumber[i], tran);
#ifdef EDM_ML_DEBUG
          edm::LogVerbatim("HGCalGeom") << "DDHGCalPassivePartial: " << glogs[i].name() << " number " << copyNumber[i]
                                        << " positioned in " << glogM.name() << " at " << tran << " with no rotation";
#endif
          ++copyNumber[i];
          zi += layerThick[i];
          thickTot += layerThick[i];
        }
        if ((std::abs(thickTot - thick) >= tol) && (!layerType.empty())) {
          if (thickTot > thick) {
            edm::LogError("HGCalGeom") << "Thickness of the partition " << thick << " is smaller than " << thickTot
                                       << ": thickness of all its components **** ERROR ****";
          } else {
            edm::LogWarning("HGCalGeom") << "Thickness of the partition " << thick << " does not match with "
                                         << thickTot << " of the components";
          }
        }
      }
    }
  }
};

static long algorithm(dd4hep::Detector& /* description */, cms::DDParsingContext& ctxt, xml_h e) {
  HGCalPassivePartial passivePartialAlgo(ctxt, e);
  return cms::s_executed;
}

DECLARE_DDCMS_DETELEMENT(DDCMS_hgcal_DDHGCalPassivePartial, algorithm)
