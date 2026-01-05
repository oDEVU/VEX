#pragma once
#include <vector>
#include <mutex>
#include <glm/glm.hpp>
#include <atomic>
#include <components/JoltSafe.hpp>

namespace vex {

/// @brief Represents a vertex used for debugging purposes.
struct DebugVertex {
    glm::vec3 position;
    glm::vec4 color;
};

#if DEBUG
/// @brief Jolt Debug renderer implementation. IT DOES NOT IMPLEMENT EVERYTHING, JUST BASIC WIREFRAME RENDERING
class VulkanPhysicsDebug : public JPH::DebugRenderer {
public:
    VulkanPhysicsDebug() { Initialize(); }

    /// @brief Draws a line between two points with a specified color.
    virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override {
        std::lock_guard<std::mutex> lock(m_mutex);

        glm::vec3 p1 = { static_cast<float>(inFrom.GetX()), static_cast<float>(inFrom.GetY()), static_cast<float>(inFrom.GetZ()) };
        glm::vec3 p2 = { static_cast<float>(inTo.GetX()), static_cast<float>(inTo.GetY()), static_cast<float>(inTo.GetZ()) };

        glm::vec4 col = {
            static_cast<float>(inColor.r) / 255.0f,
            static_cast<float>(inColor.g) / 255.0f,
            static_cast<float>(inColor.b) / 255.0f,
            static_cast<float>(inColor.a) / 255.0f
        };

        m_lines.push_back({ p1, col });
        m_lines.push_back({ p2, col });
    }

    /// @brief Draws a triangle between three points with a specified color.
    virtual void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override {
        DrawLine(inV1, inV2, inColor);
        DrawLine(inV2, inV3, inColor);
        DrawLine(inV3, inV1, inColor);
    }

    /// @brief Empty implementation of draw text since jolt requires it.
    virtual void DrawText3D(JPH::RVec3Arg inPos, const std::string_view &inString, JPH::ColorArg inColor, float inHeight) override {}

    class BatchImpl : public JPH::RefTargetVirtual {
    public:
        std::vector<JPH::Float3> mVertices;
        std::vector<JPH::uint32> mIndices;
        VulkanPhysicsDebug* mRenderer;
        std::atomic<JPH::uint32> mRefCount = 0;

        BatchImpl(const Vertex* inVertices, int inVertexCount, const JPH::uint32* inIndices, int inIndexCount, VulkanPhysicsDebug* renderer)
            : mRenderer(renderer)
        {
            mVertices.resize(inVertexCount);
            for(int i = 0; i < inVertexCount; ++i) {
                mVertices[i] = JPH::Float3(inVertices[i].mPosition.x, inVertices[i].mPosition.y, inVertices[i].mPosition.z);
            }
            if (inIndices) {
                mIndices.assign(inIndices, inIndices + inIndexCount);
            }
        }

        virtual void AddRef() override { ++mRefCount; }
        virtual void Release() override { if (--mRefCount == 0) delete this; }

        void Draw(JPH::RMat44Arg inModelMatrix, JPH::ColorArg inModelColor) {
            if (mIndices.empty()) return;

            for (size_t i = 0; i < mIndices.size(); i += 3) {
                const JPH::Float3& f1 = mVertices[mIndices[i]];
                const JPH::Float3& f2 = mVertices[mIndices[i+1]];
                const JPH::Float3& f3 = mVertices[mIndices[i+2]];

                JPH::Vec3 v1Local(f1.x, f1.y, f1.z);
                JPH::Vec3 v2Local(f2.x, f2.y, f2.z);
                JPH::Vec3 v3Local(f3.x, f3.y, f3.z);

                JPH::RVec3 v1 = inModelMatrix * v1Local;
                JPH::RVec3 v2 = inModelMatrix * v2Local;
                JPH::RVec3 v3 = inModelMatrix * v3Local;

                mRenderer->DrawLine(v1, v2, inModelColor);
                mRenderer->DrawLine(v2, v3, inModelColor);
                mRenderer->DrawLine(v3, v1, inModelColor);
            }
        }
    };

    virtual Batch CreateTriangleBatch(const Triangle *inTriangles, int inTriangleCount) override {
        std::vector<Vertex> verts(inTriangleCount * 3);
        std::vector<JPH::uint32> indices(inTriangleCount * 3);

        for(int i=0; i<inTriangleCount; ++i) {
            verts[i*3 + 0].mPosition = JPH::Float3(inTriangles[i].mV[0].mPosition.x, inTriangles[i].mV[0].mPosition.y, inTriangles[i].mV[0].mPosition.z);
            verts[i*3 + 1].mPosition = JPH::Float3(inTriangles[i].mV[1].mPosition.x, inTriangles[i].mV[1].mPosition.y, inTriangles[i].mV[1].mPosition.z);
            verts[i*3 + 2].mPosition = JPH::Float3(inTriangles[i].mV[2].mPosition.x, inTriangles[i].mV[2].mPosition.y, inTriangles[i].mV[2].mPosition.z);

            indices[i*3 + 0] = i*3 + 0;
            indices[i*3 + 1] = i*3 + 1;
            indices[i*3 + 2] = i*3 + 2;
        }

        return new BatchImpl(verts.data(), static_cast<int>(verts.size()), indices.data(), static_cast<int>(indices.size()), this);
    }

    virtual Batch CreateTriangleBatch(const Vertex *inVertices, int inVertexCount, const JPH::uint32 *inIndices, int inIndexCount) override {
        return new BatchImpl(inVertices, inVertexCount, inIndices, inIndexCount, this);
    }

    virtual void DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox &inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef &inGeometry, ECullMode inCullMode, ECastShadow inCastShadow, EDrawMode inDrawMode) override {
        if (inGeometry->mLODs.empty()) return;

        const Batch& batchRef = inGeometry->mLODs[0].mTriangleBatch;
        if (BatchImpl* batch = static_cast<BatchImpl*>(batchRef.GetPtr())) {
            batch->Draw(inModelMatrix, inModelColor);
        }
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lines.clear();
    }

    const std::vector<DebugVertex>& GetLines() { return m_lines; }

private:
    std::vector<DebugVertex> m_lines;
    std::mutex m_mutex;
};
#endif
}
