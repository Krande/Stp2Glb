//
// Created by Kristoffer on 07/05/2023.
//

#ifndef NANO_OCCT_GLTF_WRITER_H
#define NANO_OCCT_GLTF_WRITER_H

#include <filesystem>
#include <Standard_Handle.hxx>
#include <TDocStd_Document.hxx>

void to_glb_from_doc(const std::filesystem::path& glb_file, const Handle(TDocStd_Document)& doc);


#endif //NANO_OCCT_GLTF_WRITER_H
