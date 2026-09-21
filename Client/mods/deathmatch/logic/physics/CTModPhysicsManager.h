#pragma once
#include <map>
#include <vector>
#include "btBulletDynamicsCommon.h"
#include <CMatrix.h>
#include <CVector.h>
#include "Common.h"

class CClientEntity;

/**
 * STModBody
 *
 * Bullet Physics dunyasindaki rijit govde (rigid body) nesnesini ve GTA SA icindeki
 * karsilik gelen CClientEntity (CClientObject / CClientVehicle) baglantisini tutar.
 *
 * Neden var?
 * Bullet fizigi GTA dunyasindan ayri bir simulasyondur. GTA objelerinin Bullet fizigini
 * takip edebilmesi icin Bullet tarafindan her tick hesaplanan konum/rotasyon matrisinin
 * GTA nesnesine aktarilmasi, ayrica nesnenin GTA zeminine carpabilmesi icin dinamik bir
 * zemin proxy carpisma govdesinin (groundBody) saglanmasi gerekir.
 */
struct STModBody
{
    int               id = 0;
    btRigidBody*      rigidBody = nullptr;
    btCollisionShape* shape = nullptr;
    ElementID         boundElementId = INVALID_ELEMENT_ID;  // bagli GTA elementi
    btRigidBody*      groundBody = nullptr;                 // Nesnenin altindaki dinamik GTA zemin carpisicisi
    btCollisionShape* groundShape = nullptr;
    btRigidBody*      wallBody = nullptr;  // Nesnenin hareket yonundeki dinamik GTA duvar proxy carpisicisi
    btCollisionShape* wallShape = nullptr;
    bool              bFrozen = false;
    float             originalMass = 15.0f;
    btVector3         lastGroundPos = btVector3(0, 0, -9999.0f);
    bool              bWasActive = true;
};

class CTModPhysicsManager
{
public:
    static CTModPhysicsManager& GetSingleton()
    {
        static CTModPhysicsManager instance;
        return instance;
    }

    void Init();
    void Shutdown();
    void DoPulse(float fDeltaTime);  // Oyun dongusu ana guncellemesi

    // Lua ve C++ tarafindan cagrilacak kok fonksiyonlar
    int  CreateRigidBody(float x, float y, float z, float sx, float sy, float sz, float mass);
    int  CreateCustomBody(btCollisionShape* shape, float x, float y, float z, float rx = 0.0f, float ry = 0.0f, float rz = 0.0f, float mass = 0.0f);
    bool RayCast(const CVector& from, const CVector& to, CVector& outHitPoint, CVector& outHitNormal);
    int  AttachPhysics(CClientEntity* pEntity, float mass = 15.0f, float sx = 0.0f, float sy = 0.0f, float sz = 0.0f);
    bool DetachPhysics(CClientEntity* pEntity);
    bool BindBodyToElement(int id, CClientEntity* pEntity);
    void DestroyBody(int id);
    void SetBodyPosition(int id, float x, float y, float z);
    bool GetBodyPosition(int id, float& x, float& y, float& z);
    bool GetBodyRotation(int id, float& rx, float& ry, float& rz);
    bool GetBodyVelocity(int id, float& vx, float& vy, float& vz);
    void SetBodyVelocity(int id, float vx, float vy, float vz);
    void ApplyForce(int id, float fx, float fy, float fz);
    void FreezeBody(int id, bool bFreeze);
    bool IsBodyFrozen(int id);
    bool IsPlayerOnBulletGround() const { return m_bPlayerOnBulletGround; }

private:
    CTModPhysicsManager() = default;
    ~CTModPhysicsManager() = default;

    void UpdateGroundColliders();
    void UpdateWallColliders();
    void UpdatePlayerCollider();
    void UpdatePlayerGroundAndWallCollisions();
    void SyncTransformsToGTA();

    bool m_bPlayerOnBulletGround = false;

    btDefaultCollisionConfiguration*     m_collisionConfiguration = nullptr;
    btCollisionDispatcher*               m_dispatcher = nullptr;
    btBroadphaseInterface*               m_overlappingPairCache = nullptr;
    btSequentialImpulseConstraintSolver* m_solver = nullptr;
    btDiscreteDynamicsWorld*             m_dynamicsWorld = nullptr;

    // Oyuncunun Bullet objeleriyle fiziksel etkilesime girmesi (kutulari itebilmesi) icin kinematik govde
    btRigidBody*      m_playerBody = nullptr;
    btCollisionShape* m_playerShape = nullptr;

    int                      m_nextBodyId = 1;
    std::map<int, STModBody> m_bodies;
};
