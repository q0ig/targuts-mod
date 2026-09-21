#include "StdInc.h"
#include "CTModPhysicsManager.h"
#include "CClientGame.h"
#include "CClientPlayerManager.h"
#include "CClientPlayer.h"
#include "CClientEntity.h"
#include "../movement/CTModMovementManager.h"
#include "CClientObject.h"
#include "CClientVehicle.h"
#include "CElementArray.h"
#include <game/CWorld.h>
#include <game/CColPoint.h>
#include <game/CModelInfo.h>
#include <game/CPlayerPed.h>
#include <game/Task.h>
#include <game/CTaskManager.h>
#include <game/TaskTypes.h>
#include <algorithm>

/**
 * CTModPhysicsManager
 *
 * Bullet Physics simulasyon motorunu yonetir, GTA dunyasi zemin carpismalarini (ProcessLineOfSight)
 * dinamik proxy kutulariyla Bullet dunyasina tanitir ve Bullet rijit govdelerinin hesaplanan
 * koordinat/matrislerini MTA nesnelerine (CClientObject ve CClientVehicle) senkronize eder.
 *
 * Ayrica oyuncuyu kinematik bir kapsul olarak simule ederek fizik kutularini itebilmesini saglar.
 */

void CTModPhysicsManager::Init()
{
    m_collisionConfiguration = new btDefaultCollisionConfiguration();
    m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
    m_overlappingPairCache = new btDbvtBroadphase();
    m_solver = new btSequentialImpulseConstraintSolver();
    m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_overlappingPairCache, m_solver, m_collisionConfiguration);

    // GTA:SA yercekimi Z ekseninde asagi dogrudur (-9.81 m/s^2)
    m_dynamicsWorld->setGravity(btVector3(0, 0, -9.81f));

    // Solver kalitesini artir: 20 iterasyon.
    // ERP = 0.2f (stabil deger): 0.8f gibi asiri yuksek ERP degerleri penetrasyonu cozerken
    // kutulara buyuk dikey hiz ekleyerek onlarin yukari ziplamasina ve sekmesine sebep olur.
    // splitImpulse = true ile temas penetrasyonu hiz uretmeden pozisyonel olarak cozulur.
    m_dynamicsWorld->getSolverInfo().m_numIterations = 20;
    m_dynamicsWorld->getSolverInfo().m_erp = 0.2f;
    m_dynamicsWorld->getSolverInfo().m_erp2 = 0.2f;
    m_dynamicsWorld->getSolverInfo().m_splitImpulse = true;
    m_dynamicsWorld->getSolverInfo().m_splitImpulsePenetrationThreshold = -0.01f;

    // Yerel oyuncunun nesneleri itebilmesi icin kinematik kapsul carpisicisi olustur
    // Insan olceklerinde: 0.35m yaricap, 1.0m silindir yuksekligi (toplam boy ~1.70m)
    m_playerShape = new btCapsuleShapeZ(0.35f, 1.0f);
    btTransform pTrans;
    pTrans.setIdentity();
    pTrans.setOrigin(btVector3(0, 0, -9999.0f));
    btDefaultMotionState* pMotionState = new btDefaultMotionState(pTrans);
    m_playerBody = new btRigidBody(0.0f, pMotionState, m_playerShape);
    m_playerBody->setCollisionFlags(m_playerBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    m_playerBody->setActivationState(DISABLE_DEACTIVATION);
    m_playerBody->setFriction(0.8f);
    m_dynamicsWorld->addRigidBody(m_playerBody);
}

void CTModPhysicsManager::Shutdown()
{
    if (!m_dynamicsWorld)
        return;

    if (m_playerBody)
    {
        m_dynamicsWorld->removeRigidBody(m_playerBody);
        delete m_playerBody->getMotionState();
        delete m_playerBody;
        delete m_playerShape;
        m_playerBody = nullptr;
        m_playerShape = nullptr;
    }

    for (auto& [id, item] : m_bodies)
    {
        if (item.groundBody)
        {
            m_dynamicsWorld->removeRigidBody(item.groundBody);
            delete item.groundBody->getMotionState();
            delete item.groundBody;
            delete item.groundShape;
        }
        if (item.wallBody)
        {
            m_dynamicsWorld->removeRigidBody(item.wallBody);
            delete item.wallBody->getMotionState();
            delete item.wallBody;
            delete item.wallShape;
        }
        if (item.rigidBody)
        {
            m_dynamicsWorld->removeRigidBody(item.rigidBody);
            delete item.rigidBody->getMotionState();
            delete item.rigidBody;
            delete item.shape;
        }
    }
    m_bodies.clear();

    if (m_dynamicsWorld)
    {
        delete m_dynamicsWorld;
        m_dynamicsWorld = nullptr;
    }
    if (m_solver)
    {
        delete m_solver;
        m_solver = nullptr;
    }
    if (m_overlappingPairCache)
    {
        delete m_overlappingPairCache;
        m_overlappingPairCache = nullptr;
    }
    if (m_dispatcher)
    {
        delete m_dispatcher;
        m_dispatcher = nullptr;
    }
    if (m_collisionConfiguration)
    {
        delete m_collisionConfiguration;
        m_collisionConfiguration = nullptr;
    }
}

void CTModPhysicsManager::DoPulse(float fDeltaTime)
{
    if (!m_dynamicsWorld)
        return;

    // 1. Oyuncunun konumunu Bullet dunyasindaki kinematik kapsule aktar
    UpdatePlayerCollider();

    // 2. Havada veya hareket eden cisimlerin altindaki GTA zeminini tespit et ve carpisma kutusu yerlestir
    UpdateGroundColliders();

    // 3. Cisimlerin hareket ettigi yondeki GTA binalarini/duvarlarini tespit et ve dinamik carpisma engeli yerlestir
    UpdateWallColliders();

    // 4. Bullet fizik motorunu 60Hz sabit zaman adimiyla ilerlet
    m_dynamicsWorld->stepSimulation(1.0f / 60.0f, 2, 1.0f / 60.0f);

    // 5. Statik surtunme / Oturma Freni (Settling Clamp):
    // GTA SA uyumlu sabit durma: Dusuk hizdaki (speed < 0.10) cisimlerin hizini tamamen
    // sifirlayip uyku moduna gecir. Boylece engebeli/egimli GTA arazilerinde buz gibi kayma
    // ve titresim (jitter) tamamen onlenir.
    for (auto& [id, item] : m_bodies)
    {
        if (!item.rigidBody || item.bFrozen || !item.rigidBody->isActive())
            continue;

        btVector3 linVel = item.rigidBody->getLinearVelocity();
        btVector3 angVel = item.rigidBody->getAngularVelocity();
        float     linSpeed = linVel.length();
        float     angSpeed = angVel.length();

        if (linSpeed < 0.10f && angSpeed < 0.12f)
        {
            item.rigidBody->setLinearVelocity(btVector3(0, 0, 0));
            item.rigidBody->setAngularVelocity(btVector3(0, 0, 0));
        }
    }

    // 6. Bullet zemin ve yuzey carpismalarini oyuncuya uygula (Hogwarts zemininde durabilme ve yuruyebilme)
    UpdatePlayerGroundAndWallCollisions();

    // 7. Bullet'in hesapladigi yeni transformlari bagli GTA nesnelerine SetMatrix ile yaz
    SyncTransformsToGTA();

    // 8. If in source mode, sync Bullet's collision-resolved velocity back to the GTA ped
    if (CTModMovementManager::GetSingleton().GetMovementMode() == "source" && m_playerBody)
    {
        CClientPlayer* pLocalPlayer = g_pClientGame->GetPlayerManager()->GetLocalPlayer();
        if (pLocalPlayer)
        {
            // When collisions are disabled (e.g. /fly mode or noclip) or when the player is frozen,
            // Bullet's physics simulation must never inject gravity or momentum into the GTA ped.
            // Explicitly zero the GTA ped's move speed so the player stays rock-solid in midair.
            if (!pLocalPlayer->GetUsesCollision() || pLocalPlayer->IsFrozen())
            {
                pLocalPlayer->SetMoveSpeed(CVector(0.0f, 0.0f, 0.0f));
            }
            else
            {
                btVector3 finalVel = m_playerBody->getLinearVelocity();
                pLocalPlayer->SetMoveSpeed(CVector(finalVel.x() / 50.0f, finalVel.y() / 50.0f, finalVel.z() / 50.0f));
            }
        }
    }
}

int CTModPhysicsManager::CreateRigidBody(float x, float y, float z, float sx, float sy, float sz, float mass)
{
    btCollisionShape* colShape = new btBoxShape(btVector3(sx * 0.5f, sy * 0.5f, sz * 0.5f));
    btTransform       startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(x, y, z));

    btVector3 localInertia(0, 0, 0);
    if (mass != 0.0f)
        colShape->calculateLocalInertia(mass, localInertia);

    btDefaultMotionState*                    myMotionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
    // GTA:SA uyumlu sabit kutu fizigi:
    // - Yuksek lineer (0.75) ve acisal (0.85) sonumleme sayesinde cisim yere inince hizla durulur
    // - Sifir restitution (0.0) ile gereksiz ziplama ve kaucuk top gibi sekme tamamen engellenir
    // - Yuksek surtunme (2.5) ve donme surtunmesi (0.8) ile buz gibi kayma engellenir
    rbInfo.m_linearDamping = 0.75f;
    rbInfo.m_angularDamping = 0.85f;
    rbInfo.m_restitution = 0.0f;
    rbInfo.m_friction = 2.5f;
    btRigidBody* body = new btRigidBody(rbInfo);
    body->setRollingFriction(0.8f);
    body->setSpinningFriction(0.8f);
    body->setSleepingThresholds(0.12f, 0.15f);
    body->setCcdMotionThreshold(0.5f);
    body->setCcdSweptSphereRadius(0.2f);

    m_dynamicsWorld->addRigidBody(body);

    // Bu nesne icin dinamik GTA zemin carpisma proxy kutusu (6x6 metre)
    btCollisionShape* gShape = new btBoxShape(btVector3(3.0f, 3.0f, 0.5f));
    btTransform       gTrans;
    gTrans.setIdentity();
    gTrans.setOrigin(btVector3(x, y, z - 20.0f));
    btRigidBody* gBody = new btRigidBody(0.0f, new btDefaultMotionState(gTrans), gShape);
    gBody->setCollisionFlags(gBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    gBody->setActivationState(DISABLE_DEACTIVATION);
    gBody->setFriction(2.5f);
    gBody->setRestitution(0.0f);
    gBody->setRollingFriction(0.8f);
    gBody->setSpinningFriction(0.8f);
    m_dynamicsWorld->addRigidBody(gBody);

    // Bu nesne icin dinamik GTA duvar carpisma proxy kutusu (6x1x6 metre)
    btCollisionShape* wShape = new btBoxShape(btVector3(3.0f, 0.5f, 3.0f));
    btTransform       wTrans;
    wTrans.setIdentity();
    wTrans.setOrigin(btVector3(x, y, z - 9999.0f));
    btRigidBody* wBody = new btRigidBody(0.0f, new btDefaultMotionState(wTrans), wShape);
    wBody->setCollisionFlags(wBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    wBody->setActivationState(DISABLE_DEACTIVATION);
    wBody->setFriction(1.5f);
    wBody->setRestitution(0.0f);
    m_dynamicsWorld->addRigidBody(wBody);

    int       id = m_nextBodyId++;
    STModBody item;
    item.id = id;
    item.rigidBody = body;
    item.shape = colShape;
    item.boundElementId = INVALID_ELEMENT_ID;
    item.groundBody = gBody;
    item.groundShape = gShape;
    item.wallBody = wBody;
    item.wallShape = wShape;
    item.bFrozen = false;
    item.originalMass = mass;
    m_bodies[id] = item;

    return id;
}

int CTModPhysicsManager::CreateCustomBody(btCollisionShape* shape, float x, float y, float z, float rx, float ry, float rz, float mass)
{
    if (!shape || !m_dynamicsWorld)
        return -1;

    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(x, y, z));
    if (rx != 0.0f || ry != 0.0f || rz != 0.0f)
    {
        btMatrix3x3 rotMatrix;
        rotMatrix.setEulerYPR(btRadians(rz), btRadians(ry), btRadians(rx));
        startTransform.setBasis(rotMatrix);
    }

    btVector3 localInertia(0, 0, 0);
    if (mass != 0.0f)
        shape->calculateLocalInertia(mass, localInertia);

    btDefaultMotionState*                    myMotionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, shape, localInertia);
    rbInfo.m_linearDamping = 0.75f;
    rbInfo.m_angularDamping = 0.85f;
    rbInfo.m_friction = 2.5f;
    rbInfo.m_restitution = 0.0f;
    btRigidBody* body = new btRigidBody(rbInfo);
    body->setRollingFriction(0.8f);
    body->setSpinningFriction(0.8f);
    body->setSleepingThresholds(0.12f, 0.15f);

    if (mass == 0.0f)
    {
        body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
    }

    m_dynamicsWorld->addRigidBody(body);

    int       id = m_nextBodyId++;
    STModBody item;
    item.id = id;
    item.rigidBody = body;
    item.shape = shape;
    item.boundElementId = INVALID_ELEMENT_ID;
    item.groundBody = nullptr;
    item.groundShape = nullptr;
    item.wallBody = nullptr;
    item.wallShape = nullptr;
    item.bFrozen = (mass == 0.0f);
    item.originalMass = mass;
    m_bodies[id] = item;

    return id;
}

bool CTModPhysicsManager::RayCast(const CVector& from, const CVector& to, CVector& outHitPoint, CVector& outHitNormal)
{
    if (!m_dynamicsWorld)
        return false;

    btVector3 btFrom(from.fX, from.fY, from.fZ);
    btVector3 btTo(to.fX, to.fY, to.fZ);

    struct BulletWorldRay : public btCollisionWorld::ClosestRayResultCallback
    {
        btCollisionObject* m_pIgnore;
        BulletWorldRay(const btVector3& rFrom, const btVector3& rTo, btCollisionObject* pIgnore)
            : btCollisionWorld::ClosestRayResultCallback(rFrom, rTo), m_pIgnore(pIgnore)
        {
        }
        virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) override
        {
            if (rayResult.m_collisionObject == m_pIgnore)
                return 1.0f;
            return btCollisionWorld::ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
        }
    };

    BulletWorldRay ray(btFrom, btTo, m_playerBody);
    m_dynamicsWorld->rayTest(btFrom, btTo, ray);

    if (ray.hasHit())
    {
        outHitPoint = CVector(ray.m_hitPointWorld.x(), ray.m_hitPointWorld.y(), ray.m_hitPointWorld.z());
        outHitNormal = CVector(ray.m_hitNormalWorld.x(), ray.m_hitNormalWorld.y(), ray.m_hitNormalWorld.z());
        return true;
    }
    return false;
}

int CTModPhysicsManager::AttachPhysics(CClientEntity* pEntity, float mass, float sx, float sy, float sz)
{
    if (!pEntity)
        return 0;

    ElementID eId = pEntity->GetID();
    for (const auto& [id, body] : m_bodies)
    {
        if (body.boundElementId.Value() == eId.Value())
        {
            return id;
        }
    }

    // Araclarin fizige baglandiginda patlamamasi icin hasar almazlik ver
    if (pEntity->GetType() == CCLIENTVEHICLE)
    {
        static_cast<CClientVehicle*>(pEntity)->SetScriptCanBeDamaged(false);
    }

    // Boyutlar verilmediyse GTA model kutusundan (BoundingBox) otomatik hesapla
    if (sx <= 0.0f || sy <= 0.0f || sz <= 0.0f)
    {
        sx = 1.0f;
        sy = 1.0f;
        sz = 1.0f;
        unsigned short usModel = 0;
        if (pEntity->GetType() == CCLIENTOBJECT)
        {
            usModel = static_cast<CClientObject*>(pEntity)->GetModel();
        }
        else if (pEntity->GetType() == CCLIENTVEHICLE)
        {
            usModel = static_cast<CClientVehicle*>(pEntity)->GetModel();
        }

        if (usModel > 0 && g_pCore && g_pCore->GetGame())
        {
            CModelInfo* pModelInfo = g_pCore->GetGame()->GetModelInfo(usModel);
            if (pModelInfo)
            {
                CBoundingBox* pBBox = pModelInfo->GetBoundingBox();
                if (pBBox)
                {
                    sx = std::max(0.3f, pBBox->vecBoundMax.fX - pBBox->vecBoundMin.fX);
                    sy = std::max(0.3f, pBBox->vecBoundMax.fY - pBBox->vecBoundMin.fY);
                    sz = std::max(0.3f, pBBox->vecBoundMax.fZ - pBBox->vecBoundMin.fZ);
                }
            }
        }
    }

    if (mass <= 0.0f)
    {
        mass = (pEntity->GetType() == CCLIENTVEHICLE) ? 1500.0f : 25.0f;
    }

    CMatrix entityMatrix;
    pEntity->GetMatrix(entityMatrix);

    btScalar m[16];
    m[0] = entityMatrix.vRight.fX;
    m[1] = entityMatrix.vRight.fY;
    m[2] = entityMatrix.vRight.fZ;
    m[3] = 0.0f;
    m[4] = entityMatrix.vFront.fX;
    m[5] = entityMatrix.vFront.fY;
    m[6] = entityMatrix.vFront.fZ;
    m[7] = 0.0f;
    m[8] = entityMatrix.vUp.fX;
    m[9] = entityMatrix.vUp.fY;
    m[10] = entityMatrix.vUp.fZ;
    m[11] = 0.0f;
    m[12] = entityMatrix.vPos.fX;
    m[13] = entityMatrix.vPos.fY;
    m[14] = entityMatrix.vPos.fZ;
    m[15] = 1.0f;

    btTransform startTrans;
    startTrans.setFromOpenGLMatrix(m);

    btCollisionShape* colShape = new btBoxShape(btVector3(sx * 0.5f, sy * 0.5f, sz * 0.5f));
    btVector3         localInertia(0, 0, 0);
    if (mass != 0.0f)
        colShape->calculateLocalInertia(mass, localInertia);

    btDefaultMotionState*                    motionState = new btDefaultMotionState(startTrans);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, colShape, localInertia);
    rbInfo.m_linearDamping = 0.75f;
    rbInfo.m_angularDamping = 0.85f;
    rbInfo.m_restitution = 0.0f;
    rbInfo.m_friction = 2.5f;

    btRigidBody* body = new btRigidBody(rbInfo);
    body->setRollingFriction(0.8f);
    body->setSpinningFriction(0.8f);
    body->setSleepingThresholds(0.12f, 0.15f);
    body->setCcdMotionThreshold(0.5f);
    body->setCcdSweptSphereRadius(0.2f);
    m_dynamicsWorld->addRigidBody(body);

    btCollisionShape* gShape = new btBoxShape(btVector3(3.0f, 3.0f, 0.5f));
    btTransform       gTrans;
    gTrans.setIdentity();
    gTrans.setOrigin(btVector3(entityMatrix.vPos.fX, entityMatrix.vPos.fY, entityMatrix.vPos.fZ - 20.0f));
    btRigidBody* gBody = new btRigidBody(0.0f, new btDefaultMotionState(gTrans), gShape);
    gBody->setCollisionFlags(gBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    gBody->setActivationState(DISABLE_DEACTIVATION);
    gBody->setFriction(2.5f);
    gBody->setRestitution(0.0f);
    gBody->setRollingFriction(0.8f);
    gBody->setSpinningFriction(0.8f);
    m_dynamicsWorld->addRigidBody(gBody);

    // Dinamik GTA duvar proxy carpisicisi (6x1x6 metre)
    btCollisionShape* wShape = new btBoxShape(btVector3(3.0f, 0.5f, 3.0f));
    btTransform       wTrans;
    wTrans.setIdentity();
    wTrans.setOrigin(btVector3(entityMatrix.vPos.fX, entityMatrix.vPos.fY, entityMatrix.vPos.fZ - 9999.0f));
    btRigidBody* wBody = new btRigidBody(0.0f, new btDefaultMotionState(wTrans), wShape);
    wBody->setCollisionFlags(wBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    wBody->setActivationState(DISABLE_DEACTIVATION);
    wBody->setFriction(1.5f);
    wBody->setRestitution(0.0f);
    m_dynamicsWorld->addRigidBody(wBody);

    int       id = m_nextBodyId++;
    STModBody item;
    item.id = id;
    item.rigidBody = body;
    item.shape = colShape;
    item.boundElementId = eId;
    item.groundBody = gBody;
    item.groundShape = gShape;
    item.wallBody = wBody;
    item.wallShape = wShape;
    item.bFrozen = false;
    item.originalMass = mass;
    m_bodies[id] = item;

    return id;
}

bool CTModPhysicsManager::DetachPhysics(CClientEntity* pEntity)
{
    if (!pEntity)
        return false;
    ElementID eId = pEntity->GetID();
    int       foundId = 0;
    for (const auto& [id, body] : m_bodies)
    {
        if (body.boundElementId.Value() == eId.Value())
        {
            foundId = id;
            break;
        }
    }
    if (foundId > 0)
    {
        if (pEntity->GetType() == CCLIENTVEHICLE)
        {
            static_cast<CClientVehicle*>(pEntity)->SetScriptCanBeDamaged(true);
        }
        DestroyBody(foundId);
        return true;
    }
    return false;
}

bool CTModPhysicsManager::BindBodyToElement(int id, CClientEntity* pEntity)
{
    if (m_bodies.count(id))
    {
        m_bodies[id].boundElementId = pEntity ? pEntity->GetID() : INVALID_ELEMENT_ID;
        return true;
    }
    return false;
}

void CTModPhysicsManager::DestroyBody(int id)
{
    if (m_bodies.count(id))
    {
        STModBody& item = m_bodies[id];
        if (item.groundBody)
        {
            m_dynamicsWorld->removeRigidBody(item.groundBody);
            delete item.groundBody->getMotionState();
            delete item.groundBody;
            delete item.groundShape;
            item.groundBody = nullptr;
            item.groundShape = nullptr;
        }
        if (item.wallBody)
        {
            m_dynamicsWorld->removeRigidBody(item.wallBody);
            delete item.wallBody->getMotionState();
            delete item.wallBody;
            delete item.wallShape;
            item.wallBody = nullptr;
            item.wallShape = nullptr;
        }
        if (item.rigidBody)
        {
            m_dynamicsWorld->removeRigidBody(item.rigidBody);
            delete item.rigidBody->getMotionState();
            delete item.rigidBody;
            delete item.shape;
            item.rigidBody = nullptr;
            item.shape = nullptr;
        }
        m_bodies.erase(id);
    }
}

void CTModPhysicsManager::SetBodyPosition(int id, float x, float y, float z)
{
    if (m_bodies.count(id) && m_bodies[id].rigidBody)
    {
        btRigidBody* body = m_bodies[id].rigidBody;
        btTransform  trans = body->getWorldTransform();
        trans.setOrigin(btVector3(x, y, z));
        body->setWorldTransform(trans);
        body->activate(true);
        if (m_bodies[id].boundElementId.Value() != INVALID_ELEMENT_ID)
        {
            CClientEntity* pEntity = CElementIDs::GetElement(m_bodies[id].boundElementId);
            if (pEntity)
                pEntity->SetPosition(CVector(x, y, z));
        }
    }
}

bool CTModPhysicsManager::GetBodyPosition(int id, float& x, float& y, float& z)
{
    if (m_bodies.count(id) && m_bodies[id].rigidBody)
    {
        btVector3 origin = m_bodies[id].rigidBody->getWorldTransform().getOrigin();
        x = origin.x();
        y = origin.y();
        z = origin.z();
        return true;
    }
    return false;
}

bool CTModPhysicsManager::GetBodyRotation(int id, float& rx, float& ry, float& rz)
{
    if (m_bodies.count(id) && m_bodies[id].rigidBody)
    {
        btTransform trans = m_bodies[id].rigidBody->getWorldTransform();
        btScalar    m[16];
        trans.getOpenGLMatrix(m);
        CMatrix mat;
        mat.vRight = CVector(m[0], m[1], m[2]);
        mat.vFront = CVector(m[4], m[5], m[6]);
        mat.vUp = CVector(m[8], m[9], m[10]);
        mat.vPos = CVector(m[12], m[13], m[14]);
        CVector rot = mat.GetRotation();
        rx = rot.fX;
        ry = rot.fY;
        rz = rot.fZ;
        return true;
    }
    return false;
}

bool CTModPhysicsManager::GetBodyVelocity(int id, float& vx, float& vy, float& vz)
{
    if (m_bodies.count(id) && m_bodies[id].rigidBody)
    {
        btVector3 vel = m_bodies[id].rigidBody->getLinearVelocity();
        vx = vel.x();
        vy = vel.y();
        vz = vel.z();
        return true;
    }
    return false;
}

void CTModPhysicsManager::SetBodyVelocity(int id, float vx, float vy, float vz)
{
    if (m_bodies.count(id) && m_bodies[id].rigidBody)
    {
        m_bodies[id].rigidBody->activate(true);
        m_bodies[id].rigidBody->setLinearVelocity(btVector3(vx, vy, vz));
    }
}

void CTModPhysicsManager::ApplyForce(int id, float fx, float fy, float fz)
{
    if (m_bodies.count(id) && m_bodies[id].rigidBody)
    {
        btRigidBody* body = m_bodies[id].rigidBody;
        body->activate(true);
        body->applyCentralForce(btVector3(fx, fy, fz));
    }
}

void CTModPhysicsManager::FreezeBody(int id, bool bFreeze)
{
    if (m_bodies.count(id))
    {
        STModBody& body = m_bodies[id];
        body.bFrozen = bFreeze;
        if (body.rigidBody)
        {
            if (bFreeze)
            {
                body.rigidBody->setLinearVelocity(btVector3(0, 0, 0));
                body.rigidBody->setAngularVelocity(btVector3(0, 0, 0));
                body.rigidBody->setLinearFactor(btVector3(0, 0, 0));
                body.rigidBody->setAngularFactor(btVector3(0, 0, 0));
            }
            else
            {
                body.rigidBody->setLinearFactor(btVector3(1, 1, 1));
                body.rigidBody->setAngularFactor(btVector3(1, 1, 1));
                body.rigidBody->activate(true);
            }
        }
    }
}

bool CTModPhysicsManager::IsBodyFrozen(int id)
{
    if (m_bodies.count(id))
    {
        return m_bodies[id].bFrozen;
    }
    return false;
}

void CTModPhysicsManager::UpdatePlayerCollider()
{
    if (!m_playerBody || !g_pClientGame)
        return;

    CClientPlayerManager* pPlayerManager = g_pClientGame->GetPlayerManager();
    if (!pPlayerManager)
        return;

    CClientPlayer* pLocalPlayer = pPlayerManager->GetLocalPlayer();
    if (!pLocalPlayer || !pLocalPlayer->GetGamePlayer() || pLocalPlayer->IsDead())
    {
        btTransform trans;
        trans.setIdentity();
        trans.setOrigin(btVector3(0, 0, -9999.0f));
        m_playerBody->setWorldTransform(trans);
        m_playerBody->setLinearVelocity(btVector3(0, 0, 0));
        m_dynamicsWorld->updateSingleAabb(m_playerBody);
        return;
    }

    bool bSourceMode = (CTModMovementManager::GetSingleton().GetMovementMode() == "source");

    // In source mode, player is a dynamic physics capsule, we don't sync GTA -> Bullet, we sync Bullet -> GTA.
    static bool s_wasSourceMode = false;

    if (bSourceMode)
    {
        if (!s_wasSourceMode)
        {
            m_dynamicsWorld->removeRigidBody(m_playerBody);
            m_playerBody->setCollisionFlags(m_playerBody->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
            m_playerBody->setAngularFactor(btVector3(0, 0, 0));  // No rotation
            m_playerBody->setRestitution(0.0f);
            m_playerBody->setFriction(0.0f);  // MovementManager handles friction
            btVector3 inertia(0, 0, 0);
            m_playerShape->calculateLocalInertia(80.0f, inertia);
            m_playerBody->setMassProps(80.0f, inertia);
            m_dynamicsWorld->addRigidBody(m_playerBody);
            s_wasSourceMode = true;
        }

        // Make sure it doesn't sleep while playing
        m_playerBody->activate(true);

        // FLY & NOCLIP PROTECTION:
        // In Fly mode or noclip, MTA disables player collisions (or freezes the ped).
        // If we continue dynamic simulation here, Bullet's 9.8m/s^2 gravity will accelerate the 80kg capsule
        // downward in midair during stepSimulation, and the subsequent sync call would drag the GTA ped down.
        // We explicitly zero gravity and velocities, synchronize the Bullet capsule to the GTA ped position,
        // and return immediately so the player hovers stably without dropping.
        if (!pLocalPlayer->GetUsesCollision() || pLocalPlayer->IsFrozen())
        {
            m_playerBody->setGravity(btVector3(0, 0, 0));
            m_playerBody->setLinearVelocity(btVector3(0, 0, 0));
            m_playerBody->setAngularVelocity(btVector3(0, 0, 0));
            CVector gtaPos;
            pLocalPlayer->GetPosition(gtaPos);
            btTransform t = m_playerBody->getWorldTransform();
            t.setOrigin(btVector3(gtaPos.fX, gtaPos.fY, gtaPos.fZ));
            m_playerBody->setWorldTransform(t);
            return;
        }
        else
        {
            // Restore normal Bullet world gravity when collisions are active
            m_playerBody->setGravity(m_dynamicsWorld->getGravity());
        }

        // --- GROUND RAYCAST ---
        // Kapsulun zemine batip titremesini engellemek icin asagi dogru raycast at
        btVector3 pos = m_playerBody->getWorldTransform().getOrigin();
        btVector3 rayStart = pos;
        btVector3 rayEnd = pos - btVector3(0, 0, 1.0f);  // Kapsul yarisi + 0.15m pay

        struct GroundRayCallback : public btCollisionWorld::ClosestRayResultCallback
        {
            btRigidBody* me;
            GroundRayCallback(const btVector3& s, const btVector3& e, btRigidBody* me) : btCollisionWorld::ClosestRayResultCallback(s, e), me(me) {}
            virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
            {
                if (rayResult.m_collisionObject == me)
                    return 1.0f;  // ignore self
                return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
            }
        };

        GroundRayCallback rayCallback(rayStart, rayEnd, m_playerBody);
        m_dynamicsWorld->rayTest(rayStart, rayEnd, rayCallback);

        if (rayCallback.hasHit())
        {
            // Zemin altimizdaysa ve batmissak kapsulu yuzeye kaldir
            float distToGround = pos.z() - rayCallback.m_hitPointWorld.z();
            float desiredDist = 0.85f;  // Kapsul silindir boyu yarisi (0.5) + kure yaricapi (0.35) = 0.85m
            if (distToGround < desiredDist)
            {
                pos.setZ(rayCallback.m_hitPointWorld.z() + desiredDist);
                btTransform t = m_playerBody->getWorldTransform();
                t.setOrigin(pos);
                m_playerBody->setWorldTransform(t);
            }
        }

        // --- VELOCITY SYNC ---
        // WASD girdilerinden gelen hiz vektorunu dogrudan kapsulun linear velocity'sine aktar
        CVector pedVel;
        pLocalPlayer->GetMoveSpeed(pedVel);

        btVector3 currentVel = m_playerBody->getLinearVelocity();
        // X ve Y (yatay) hizi GTA'dan al, Z (dikey) hizi eger ziplama yoksa Bullet'in yercekimine birak
        float targetVx = pedVel.fX * 50.0f;
        float targetVy = pedVel.fY * 50.0f;
        float targetVz = currentVel.z();  // Gravity koru

        // Eger Bullet zemini yoksa (yani standart GTA haritasi uzerindeysek) ve GTA motoru zeminde oldugumuzu soyluyorsa,
        // yercekimini iptal et ve kapsulu GTA zemini uzerinde tut ki sonsuzluga dusmesin!
        if (!m_bPlayerOnBulletGround && pLocalPlayer->IsOnGround(true))
        {
            targetVz = 0.0f;

            // Kapsulun Z konumunu GTA pedinin Z konumuna sabitle (pelvis - 0.05m offset ile)
            CVector gtaPos;
            pLocalPlayer->GetPosition(gtaPos);
            pos.setZ(gtaPos.fZ - 0.05f);

            btTransform t = m_playerBody->getWorldTransform();
            t.setOrigin(pos);
            m_playerBody->setWorldTransform(t);
        }
        else if (m_bPlayerOnBulletGround)
        {
            // FBX sehir haritasi uzerinde dururken yercekiminin kapsulu zeminin altina cekmesini engelle
            if (targetVz < 0.0f)
                targetVz = 0.0f;

            CVector gtaPos;
            pLocalPlayer->GetPosition(gtaPos);
            pos.setZ(gtaPos.fZ - 0.05f);

            btTransform t = m_playerBody->getWorldTransform();
            t.setOrigin(pos);
            m_playerBody->setWorldTransform(t);
        }

        // Eger GTA ziplama hizi (yukari) varsa onu da aktar
        if (pedVel.fZ > 0.01f)
        {
            targetVz = pedVel.fZ * 50.0f;
            // Tuketildi, bir dahaki frame'de gravity tekrar devralacak
            pLocalPlayer->SetMoveSpeed(CVector(pedVel.fX, pedVel.fY, 0.0f));
        }

        m_playerBody->setLinearVelocity(btVector3(targetVx, targetVy, targetVz));

        // Sync position back to GTA ped so camera and weapons work properly
        CVector pedPos(pos.x(), pos.y(), pos.z() + 0.05f);  // +0.05 to place pelvis correctly
        pLocalPlayer->SetPosition(pedPos);
        return;  // Skip kinematic proxy update
    }

    // Default mode: GTA controls position, Bullet is a kinematic proxy
    if (s_wasSourceMode)
    {
        m_dynamicsWorld->removeRigidBody(m_playerBody);
        m_playerBody->setCollisionFlags(m_playerBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        // CRITICAL: Kinematic objects MUST have 0 mass in Bullet!
        m_playerBody->setMassProps(0.0f, btVector3(0, 0, 0));
        m_playerBody->setFriction(0.8f);  // Restore normal friction
        m_dynamicsWorld->addRigidBody(m_playerBody);
        s_wasSourceMode = false;
    }

    CVector pedPos;
    pLocalPlayer->GetPosition(pedPos);

    CVector pedVel;
    pLocalPlayer->GetMoveSpeed(pedVel);

    // Kapsul boyutu: 0.35m yaricap, 1.0m silindir boyu -> toplam 1.70m yukseklik
    // GTA'da pedPos.fZ zaten pelvis/govde merkezindedir (~1.0m zemin ustunde).
    // Kapsul merkezini pedPos.fZ - 0.05f yaparak alt tabanin ayak seviyesine denk gelmesini sagla.
    btTransform trans;
    trans.setIdentity();
    trans.setOrigin(btVector3(pedPos.fX, pedPos.fY, pedPos.fZ - 0.05f));
    if (m_playerBody->getMotionState())
        m_playerBody->getMotionState()->setWorldTransform(trans);
    m_playerBody->setWorldTransform(trans);

    // Kinematik kapsule oyuncunun GTA hareket hizini kontrollu olarak aktar
    // Neden Z sifirlaniyor ve hiz sinirlaniyor?
    // Bullet kinematik objeleri sonsuz kutleye sahiptir. Karakter yururken veya ziplarken
    // yukari dogru pedVel.fZ kinematik govdeye aktarilirsa, Bullet'in temas cozucusu (solver)
    // kutulara muazzam bir yukari itme uygulayarak kutulari gokyuzune firlatir.
    // Dikey hizi 0 yaparak ve yatay itme hizini maksimum ~4 m/s ile sinirlandirarak
    // oyuncunun kutulari dogal bir sekilde itebilmesi saglanir.
    float horizSpeed = std::sqrt(pedVel.fX * pedVel.fX + pedVel.fY * pedVel.fY);
    float cappedSpeed = std::min(horizSpeed, 0.08f);
    float pushVx = 0.0f;
    float pushVy = 0.0f;
    if (horizSpeed > 0.0001f)
    {
        pushVx = (pedVel.fX / horizSpeed) * cappedSpeed * 50.0f;
        pushVy = (pedVel.fY / horizSpeed) * cappedSpeed * 50.0f;
    }
    btVector3 bulletVel(pushVx, pushVy, 0.0f);
    m_playerBody->setLinearVelocity(bulletVel);
    m_dynamicsWorld->updateSingleAabb(m_playerBody);

    // Sadece yerel oyuncu hareket ediyorken ve temas mesafesindeyse (1.4m) uyuyan cisimleri uyandir.
    // Oyuncu dururken yakindaki cisimlerin yapay olarak uyanik tutulmasi titresim ve kaymaya neden oluyordu.
    if (horizSpeed > 0.01f)
    {
        for (auto& [id, item] : m_bodies)
        {
            if (item.rigidBody && !item.bFrozen && !item.rigidBody->isActive())
            {
                btVector3 bPos = item.rigidBody->getWorldTransform().getOrigin();
                float     dx = bPos.x() - pedPos.fX;
                float     dy = bPos.y() - pedPos.fY;
                float     dz = bPos.z() - pedPos.fZ;
                if ((dx * dx + dy * dy) < 2.0f && fabs(dz) < 1.8f)
                {
                    item.rigidBody->activate(true);
                }
            }
        }
    }
}

void CTModPhysicsManager::UpdatePlayerGroundAndWallCollisions()
{
    // Onceki karenin Bullet zemin durumunu sakla (bStepping adim/kaldirim toleransi icin)
    bool bWasOnBulletGround = m_bPlayerOnBulletGround;
    m_bPlayerOnBulletGround = false;

    if (!m_dynamicsWorld || !g_pClientGame)
        return;

    CClientPlayerManager* pPlayerManager = g_pClientGame->GetPlayerManager();
    if (!pPlayerManager)
        return;

    CClientPlayer* pLocalPlayer = pPlayerManager->GetLocalPlayer();
    if (!pLocalPlayer || !pLocalPlayer->GetGamePlayer() || pLocalPlayer->IsDead() || pLocalPlayer->IsInVehicle())
        return;

    // Fly modu veya noclip aktifken carpisma mudahalesini devre disi birak (oyuncu havada serbestce ucsun)
    if (!pLocalPlayer->GetUsesCollision() || pLocalPlayer->IsFrozen())
        return;

    CVector pedPos;
    pLocalPlayer->GetPosition(pedPos);

    CVector pedVel;
    pLocalPlayer->GetMoveSpeed(pedVel);

    // Kareler arasi oyuncu konum takibi (tunelleme engelleme ve guvenli isin baslangici)
    static CVector s_lastFramePedPos = pedPos;
    static bool    s_bHasLastPedPos = false;
    if (!s_bHasLastPedPos)
    {
        s_lastFramePedPos = pedPos;
        s_bHasLastPedPos = true;
    }

    struct BulletIgnorePlayerRay : public btCollisionWorld::ClosestRayResultCallback
    {
        btCollisionObject* m_pIgnore;
        BulletIgnorePlayerRay(const btVector3& from, const btVector3& to, btCollisionObject* pIgnore)
            : btCollisionWorld::ClosestRayResultCallback(from, to), m_pIgnore(pIgnore)
        {
        }

        virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) override
        {
            if (rayResult.m_collisionObject == m_pIgnore)
                return 1.0f;
            return btCollisionWorld::ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
        }
    };

    // ------------------------------------------------------------------------
    // 1. ZEMIN VE OTO CARPISMA TESPITI (Multi-Point Bullet BVH Raycasts)
    // ------------------------------------------------------------------------
    // RenderWare limitlerinden (COL dosya boyutlari, max poligon limiti vb.) tamamen
    // bagimsiz olarak Bullet Physics BVH agi uzerinde zemin tespiti yapilir.
    // Merkez ve 4 capraz sondaj noktasiyla (0.28m yaricap) kaldirim ve merdiven kenarlarinda
    // titresim olusmasi engellenir.
    const float groundSampleOffsets[5][2] = {
        {  0.0f,   0.0f },
        {  0.28f,  0.0f },
        { -0.28f,  0.0f },
        {  0.0f,   0.28f },
        {  0.0f,  -0.28f }
    };

    float maxGroundHitZ = -99999.0f;
    bool  bFoundGround = false;

    // Isin baslangic yuksekligi: Oyuncunun bas seviyesinin ustu veya onceki karenin yuksekligi
    // Yuksek hizli serbest dususlerde veya yerin icine batildiginda tunelleme engellenir.
    float rayStartZ = std::max(pedPos.fZ + 1.2f, s_lastFramePedPos.fZ + 0.6f);
    float rayEndZ   = pedPos.fZ - 12.0f;

    for (int i = 0; i < 5; ++i)
    {
        btVector3 rFrom(pedPos.fX + groundSampleOffsets[i][0], pedPos.fY + groundSampleOffsets[i][1], rayStartZ);
        btVector3 rTo(pedPos.fX + groundSampleOffsets[i][0], pedPos.fY + groundSampleOffsets[i][1], rayEndZ);

        BulletIgnorePlayerRay gRay(rFrom, rTo, m_playerBody);
        m_dynamicsWorld->rayTest(rFrom, rTo, gRay);

        if (gRay.hasHit())
        {
            float     hitZ = gRay.m_hitPointWorld.z();
            btVector3 norm = gRay.m_hitNormalWorld;

            // Zemin tespit kriteri:
            // 1. Yuzey normali yukari dogru olmalidir (norm.z >= 0.2f). Bu sayede tavanlar
            //    (norm.z < 0) ve dikey duvarlar (norm.z ~ 0) asla zemin sanilmaz.
            // 2. hitZ <= rayStartZ ve bas seviyesinin altinda (veya onceki kare seviyesinde) olmalidir.
            float maxAllowedZ = std::max(pedPos.fZ + 0.85f, s_lastFramePedPos.fZ + 0.2f);
            if (norm.z() >= 0.2f && hitZ <= maxAllowedZ && hitZ > maxGroundHitZ)
            {
                maxGroundHitZ = hitZ;
                bFoundGround = true;
            }
        }
    }

    float feetZ = pedPos.fZ - 1.0f;

    if (bFoundGround)
    {
        // 1. Havadan inis: Ayaklar zemine yaklastiginda veya zemine battiginda
        bool bLanded = (feetZ <= maxGroundHitZ + 0.25f && pedPos.fZ >= maxGroundHitZ - 2.5f);
        // 2. Basamak/Kaldirim: Zaten zemindeyken 0.65m yukseklige kadar adim atabilme
        bool bStepping = (bWasOnBulletGround && std::abs(feetZ - maxGroundHitZ) <= 0.65f);

        // Ziplama ivmesi yokken (pedVel.fZ <= 0.15f) zemine kenetle
        if ((bLanded || bStepping) && pedVel.fZ <= 0.15f)
        {
            m_bPlayerOnBulletGround = true;

            // Oyuncunun ayaklarini zemine tam oturt (pelvis = zemin + 1.0m)
            pedPos.fZ = maxGroundHitZ + 1.0f;
            pLocalPlayer->SetPosition(pedPos);

            // Asagi dogru dusus hizini sifirla
            if (pedVel.fZ < 0.0f)
            {
                pedVel.fZ = 0.0f;
                pLocalPlayer->SetMoveSpeed(pedVel);
            }

            // Bullet kinematik kapsulunu de zemin ustune hizala
            if (m_playerBody)
            {
                btTransform t = m_playerBody->getWorldTransform();
                btVector3   org = t.getOrigin();
                org.setZ(pedPos.fZ - 0.05f);
                t.setOrigin(org);
                m_playerBody->setWorldTransform(t);

                btVector3 bVel = m_playerBody->getLinearVelocity();
                if (bVel.z() < 0.0f)
                {
                    bVel.setZ(0.0f);
                    m_playerBody->setLinearVelocity(bVel);
                }
            }

            // GTA SA motoruna karakterin saglam zeminde durdugunu bildir (CPed::SetIsStanding)
            // Bu sayede GTA SA yercekimi ve serbest dusus animasyonu uygulamaz
            if (pLocalPlayer->GetGamePlayer())
            {
                pLocalPlayer->GetGamePlayer()->SetIsStanding(true);
            }

            // Havadaki dusus / cirpinma animasyonunu ve inis gecikmesini aninda iptal et
            CTaskManager* pTaskManager = pLocalPlayer->GetTaskManager();
            if (pTaskManager)
            {
                CTask* pSimplest = pTaskManager->GetSimplestActiveTask();
                if (pSimplest)
                {
                    int taskType = pSimplest->GetTaskType();
                    if (taskType == TASK_SIMPLE_IN_AIR || taskType == TASK_SIMPLE_LAND)
                    {
                        pSimplest->MakeAbortable(pLocalPlayer->GetGamePlayer(), ABORT_PRIORITY_URGENT, nullptr);
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------------------
    // 2. DUVAR VE BINA CARPISMASI (8 Yonlu Radyal Sondaj & Disari Itme)
    // ------------------------------------------------------------------------
    // Oyuncunun 0.42m silindir yaricapi cevresi 8 aciyla ve 3 farkli yukseklikte
    // (diz/kaldirim pedPos.fZ - 0.4f, bel pedPos.fZ, gogus pedPos.fZ + 0.5f) taranir.
    // Herhangi bir bina veya engele carparsa oyuncu disa itilir ve duvardan akici kayma saglanir.
    const float playerRadius = 0.42f;
    const float probeDistance = 0.65f;
    const int   numDirections = 8;
    const float angles[numDirections] = {
        0.0f,
        0.785398f,  // 45 deg
        1.570796f,  // 90 deg
        2.356194f,  // 135 deg
        3.141593f,  // 180 deg
        3.926991f,  // 225 deg
        4.712389f,  // 270 deg
        5.497787f   // 315 deg
    };

    float probeHeights[3] = { pedPos.fZ - 0.4f, pedPos.fZ, pedPos.fZ + 0.5f };

    for (int h = 0; h < 3; ++h)
    {
        for (int i = 0; i < numDirections; ++i)
        {
            float dirX = cosf(angles[i]);
            float dirY = sinf(angles[i]);

            btVector3 wFrom(pedPos.fX, pedPos.fY, probeHeights[h]);
            btVector3 wTo(pedPos.fX + dirX * probeDistance, pedPos.fY + dirY * probeDistance, probeHeights[h]);

            BulletIgnorePlayerRay wRay(wFrom, wTo, m_playerBody);
            m_dynamicsWorld->rayTest(wFrom, wTo, wRay);

            if (wRay.hasHit())
            {
                btVector3 norm = wRay.m_hitNormalWorld;
                float     normLen = sqrtf(norm.x() * norm.x() + norm.y() * norm.y());
                if (normLen > 0.1f)
                {
                    float nx = norm.x() / normLen;
                    float ny = norm.y() / normLen;
                    float hitDist = wRay.m_closestHitFraction * probeDistance;

                    if (hitDist < playerRadius)
                    {
                        float pushOut = playerRadius - hitDist;
                        pedPos.fX += nx * pushOut;
                        pedPos.fY += ny * pushOut;
                        pLocalPlayer->SetPosition(pedPos);

                        // Duvara dogru olan hareket hizini duvardan kayacak sekilde sonumle
                        float dot = pedVel.fX * nx + pedVel.fY * ny;
                        if (dot < 0.0f)
                        {
                            pedVel.fX -= dot * nx;
                            pedVel.fY -= dot * ny;
                            pLocalPlayer->SetMoveSpeed(pedVel);
                        }
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------------------
    // 3. KARELER ARASI KESINTISIZ CARPISMA (Continuous Sweep from Last Safe Position)
    // ------------------------------------------------------------------------
    static CVector s_lastSafePedPos = pedPos;
    static bool    s_bHasLastSafePos = false;
    if (!s_bHasLastSafePos)
    {
        s_lastSafePedPos = pedPos;
        s_bHasLastSafePos = true;
    }

    float moveDistSq = (pedPos.fX - s_lastSafePedPos.fX) * (pedPos.fX - s_lastSafePedPos.fX) +
                       (pedPos.fY - s_lastSafePedPos.fY) * (pedPos.fY - s_lastSafePedPos.fY);

    if (moveDistSq > 0.0004f && moveDistSq < 16.0f)
    {
        btVector3 sweepFrom(s_lastSafePedPos.fX, s_lastSafePedPos.fY, pedPos.fZ);
        btVector3 sweepTo(pedPos.fX, pedPos.fY, pedPos.fZ);

        BulletIgnorePlayerRay sweepRay(sweepFrom, sweepTo, m_playerBody);
        m_dynamicsWorld->rayTest(sweepFrom, sweepTo, sweepRay);

        if (sweepRay.hasHit())
        {
            btVector3 norm = sweepRay.m_hitNormalWorld;
            float     normLen = sqrtf(norm.x() * norm.x() + norm.y() * norm.y());
            if (normLen > 0.1f)
            {
                float     nx = norm.x() / normLen;
                float     ny = norm.y() / normLen;
                btVector3 hitPt = sweepRay.m_hitPointWorld;

                pedPos.fX = hitPt.x() + nx * playerRadius;
                pedPos.fY = hitPt.y() + ny * playerRadius;
                pLocalPlayer->SetPosition(pedPos);

                float dot = pedVel.fX * nx + pedVel.fY * ny;
                if (dot < 0.0f)
                {
                    pedVel.fX -= dot * nx;
                    pedVel.fY -= dot * ny;
                    pLocalPlayer->SetMoveSpeed(pedVel);
                }
            }
        }
    }
    s_lastSafePedPos = pedPos;
    s_lastFramePedPos = pedPos;
}

void CTModPhysicsManager::UpdateGroundColliders()
{
    if (!g_pCore || !g_pCore->GetGame() || !g_pCore->GetGame()->GetWorld())
        return;
    CWorld* pWorld = g_pCore->GetGame()->GetWorld();

    SLineOfSightFlags flags;
    flags.bCheckBuildings = true;
    flags.bCheckVehicles = false;
    flags.bCheckPeds = false;
    flags.bCheckObjects = true;
    flags.bCheckDummies = true;
    flags.bSeeThroughStuff = false;
    flags.bIgnoreSomeObjectsForCamera = false;

    for (auto& [id, item] : m_bodies)
    {
        if (!item.rigidBody || !item.groundBody || item.bFrozen)
            continue;

        // Eger cisim uykudaysa zemin proxy'sini hic elleme, stabil kalsin
        if (!item.rigidBody->isActive())
            continue;

        btTransform trans = item.rigidBody->getWorldTransform();
        btVector3   origin = trans.getOrigin();

        if (origin.z() < -90.0f)
        {
            origin.setZ(-80.0f);
            trans.setOrigin(origin);
            item.rigidBody->setWorldTransform(trans);
            item.rigidBody->setLinearVelocity(btVector3(0, 0, 0));
        }

        // Zemin proxy'sinin mevcuttaki konumunu kontrol et:
        // gShape 6x6 metredir (half extents 3.0f, 3.0f, 0.5f).
        // Eger nesne zemin proxy'sinin merkezinden 1.0 metreden fazla uzaklasmamissa
        // ve dikeyde de 1.5 metreden fazla ayrilmamissa, zemin proxy'si zaten nesnenin altindadir!
        // Proxy'yi her kare hareket ettirmek temas cozucusunde titresim ve yukari ziplama (bounce) yaratir.
        float hDistSq = (origin.x() - item.lastGroundPos.x()) * (origin.x() - item.lastGroundPos.x()) +
                        (origin.y() - item.lastGroundPos.y()) * (origin.y() - item.lastGroundPos.y());
        float vDist = fabs(origin.z() - (item.lastGroundPos.z() + 0.5f));

        if (hDistSq < 1.0f && vDist < 1.5f && item.lastGroundPos.z() > -9000.0f)
        {
            // Zemin proxy'si zaten uygun yerde, hareket ettirmeye gerek yok!
            continue;
        }

        btVector3 aabbMin, aabbMax;
        item.rigidBody->getAabb(aabbMin, aabbMax);

        // Raycast baslangic ve bitis noktalari: Nesnenin tepesinden baslayip altina dogru ara
        CVector    vecStart(origin.x(), origin.y(), aabbMax.z() + 0.2f);
        CVector    vecEnd(origin.x(), origin.y(), aabbMin.z() - 30.0f);
        CColPoint* pColPoint = nullptr;
        CEntity*   pColEntity = nullptr;

        // Raycast'in cismin kendi carpisma modeline carpmasini engellemek icin bagli nesneyi yok say
        CClientEntity* pBoundEntity = (item.boundElementId.Value() != INVALID_ELEMENT_ID) ? CElementIDs::GetElement(item.boundElementId) : nullptr;
        CEntity*       pIgnored = pBoundEntity ? pBoundEntity->GetGameEntity() : nullptr;
        if (pIgnored)
            pWorld->IgnoreEntity(pIgnored);

        bool bHit = pWorld->ProcessLineOfSight(&vecStart, &vecEnd, &pColPoint, &pColEntity, flags);

        if (pIgnored)
            pWorld->IgnoreEntity(nullptr);

        if (bHit && pColPoint)
        {
            CVector hitPos = pColPoint->GetPosition();
            CVector hitNorm = pColPoint->GetNormal();
            pColPoint->Destroy();

            btTransform gTrans;
            gTrans.setIdentity();

            // Zemin normal vektorune gore carpisma kutusunun acisini egimli arazilere ve rampalara uyarla
            btVector3 up(0, 0, 1.0f);
            btVector3 normal(hitNorm.fX, hitNorm.fY, hitNorm.fZ);
            if (normal.length2() > 0.001f)
            {
                normal.normalize();
                btVector3 axis = up.cross(normal);
                float     dot = up.dot(normal);
                if (axis.length2() > 0.0001f)
                {
                    gTrans.setRotation(btQuaternion(axis.normalized(), acos(std::clamp(dot, -1.0f, 1.0f))));
                }
                else if (dot < -0.99f)
                {
                    gTrans.setRotation(btQuaternion(btVector3(1.0f, 0, 0), 3.14159265f));
                }
            }

            // Kutu yarim kalinligi 0.5m oldugundan yuzeyin altina normal yonunde yarim kalinlik kadar indir
            btVector3 center = btVector3(hitPos.fX, hitPos.fY, hitPos.fZ) - normal * 0.5f;
            gTrans.setOrigin(center);
            if (item.groundBody->getMotionState())
                item.groundBody->getMotionState()->setWorldTransform(gTrans);
            item.groundBody->setWorldTransform(gTrans);
            m_dynamicsWorld->updateSingleAabb(item.groundBody);
            item.lastGroundPos = center;
        }
        else
        {
            // Cismin altinda zemin yoksa (yuksekten dusus veya ucarken), hayalet zemin kalmamasi icin kutuyu uzaklastir
            btTransform gTrans;
            gTrans.setIdentity();
            gTrans.setOrigin(btVector3(0, 0, -9999.0f));
            if (item.groundBody->getMotionState())
                item.groundBody->getMotionState()->setWorldTransform(gTrans);
            item.groundBody->setWorldTransform(gTrans);
            m_dynamicsWorld->updateSingleAabb(item.groundBody);
            item.lastGroundPos = btVector3(0, 0, -9999.0f);
        }
    }
}

void CTModPhysicsManager::UpdateWallColliders()
{
    if (!g_pCore || !g_pCore->GetGame() || !g_pCore->GetGame()->GetWorld())
        return;
    CWorld* pWorld = g_pCore->GetGame()->GetWorld();

    SLineOfSightFlags flags;
    flags.bCheckBuildings = true;
    flags.bCheckVehicles = false;
    flags.bCheckPeds = false;
    flags.bCheckObjects = true;
    flags.bCheckDummies = true;
    flags.bSeeThroughStuff = false;
    flags.bIgnoreSomeObjectsForCamera = false;

    for (auto& [id, item] : m_bodies)
    {
        if (!item.rigidBody || !item.wallBody || item.bFrozen)
            continue;

        btTransform trans = item.rigidBody->getWorldTransform();
        btVector3   origin = trans.getOrigin();
        btVector3   linVel = item.rigidBody->getLinearVelocity();

        float horizSpeedSq = linVel.x() * linVel.x() + linVel.y() * linVel.y();
        if (horizSpeedSq > 0.04f)
        {
            float horizSpeed = std::sqrt(horizSpeedSq);
            float dirX = linVel.x() / horizSpeed;
            float dirY = linVel.y() / horizSpeed;

            // Hiz buyuklugune bagli on goruse gore raycast mesafesi (1.2m ile 3.5m arasi)
            float lookahead = std::clamp(horizSpeed * 0.4f, 1.2f, 3.5f);

            CVector vecStart(origin.x(), origin.y(), origin.z());
            CVector vecEnd(origin.x() + dirX * lookahead, origin.y() + dirY * lookahead, origin.z());

            CColPoint* pColPoint = nullptr;
            CEntity*   pColEntity = nullptr;

            CClientEntity* pBoundEntity = (item.boundElementId.Value() != INVALID_ELEMENT_ID) ? CElementIDs::GetElement(item.boundElementId) : nullptr;
            CEntity*       pIgnored = pBoundEntity ? pBoundEntity->GetGameEntity() : nullptr;
            if (pIgnored)
                pWorld->IgnoreEntity(pIgnored);

            bool bHit = pWorld->ProcessLineOfSight(&vecStart, &vecEnd, &pColPoint, &pColEntity, flags);

            if (pIgnored)
                pWorld->IgnoreEntity(nullptr);

            if (bHit && pColPoint)
            {
                CVector hitPos = pColPoint->GetPosition();
                CVector hitNorm = pColPoint->GetNormal();
                pColPoint->Destroy();

                btTransform wTrans;
                wTrans.setIdentity();

                // Duvar normaline gore proxy kutusunu yonlendir
                btVector3 normal(hitNorm.fX, hitNorm.fY, 0.0f);
                if (normal.length2() > 0.001f)
                    normal.normalize();
                else
                    normal = btVector3(-dirX, -dirY, 0.0f);

                btVector3 forward(0, 1.0f, 0);
                btVector3 axis = forward.cross(normal);
                float     dot = forward.dot(normal);
                if (axis.length2() > 0.0001f)
                {
                    wTrans.setRotation(btQuaternion(axis.normalized(), acos(std::clamp(dot, -1.0f, 1.0f))));
                }
                else if (dot < -0.99f)
                {
                    wTrans.setRotation(btQuaternion(btVector3(0, 0, 1.0f), 3.14159265f));
                }

                // Duvar yuzeyinin yarim kalinlik (0.5m) gerisine yerlestir
                btVector3 center = btVector3(hitPos.fX, hitPos.fY, hitPos.fZ) - normal * 0.5f;
                wTrans.setOrigin(center);

                if (item.wallBody->getMotionState())
                    item.wallBody->getMotionState()->setWorldTransform(wTrans);
                item.wallBody->setWorldTransform(wTrans);
                m_dynamicsWorld->updateSingleAabb(item.wallBody);
                continue;
            }
        }

        // Duvar algilanmadiysa kutuyu uzaklastir
        btTransform wTrans;
        wTrans.setIdentity();
        wTrans.setOrigin(btVector3(0, 0, -9999.0f));
        if (item.wallBody->getMotionState())
            item.wallBody->getMotionState()->setWorldTransform(wTrans);
        item.wallBody->setWorldTransform(wTrans);
        m_dynamicsWorld->updateSingleAabb(item.wallBody);
    }
}

void CTModPhysicsManager::SyncTransformsToGTA()
{
    for (auto& [id, item] : m_bodies)
    {
        if (!item.rigidBody || item.boundElementId.Value() == INVALID_ELEMENT_ID)
            continue;

        // Eger cisim uykudaysa ve onceki karede zaten senkronize edildiyse,
        // GTA SA nesnesine her kare SetMatrix cagrisi yapip titresim yaratma!
        bool bIsActive = item.rigidBody->isActive();
        if (!bIsActive && !item.bWasActive)
            continue;
        item.bWasActive = bIsActive;

        CClientEntity* pEntity = CElementIDs::GetElement(item.boundElementId);
        if (!pEntity)
        {
            item.boundElementId = INVALID_ELEMENT_ID;
            continue;
        }

        btTransform trans = item.rigidBody->getWorldTransform();
        btScalar    m[16];
        trans.getOpenGLMatrix(m);

        CMatrix mat;
        mat.vRight = CVector(m[0], m[1], m[2]);
        mat.vFront = CVector(m[4], m[5], m[6]);
        mat.vUp = CVector(m[8], m[9], m[10]);
        mat.vPos = CVector(m[12], m[13], m[14]);

        if (pEntity->GetType() == CCLIENTOBJECT)
        {
            CClientObject* pObj = static_cast<CClientObject*>(pEntity);
            pEntity->SetMatrix(mat);
            pObj->UpdateStreamPosition(mat.vPos);
        }
        else if (pEntity->GetType() == CCLIENTVEHICLE)
        {
            CClientVehicle* pVeh = static_cast<CClientVehicle*>(pEntity);
            pVeh->SetScriptCanBeDamaged(false);
            if (pVeh->GetHealth() < 300.0f)
                pVeh->SetHealth(1000.0f);
            pEntity->SetMatrix(mat);
            btVector3 vel = item.rigidBody->getLinearVelocity();
            float     vx = std::clamp(vel.x() / 50.0f, -1.5f, 1.5f);
            float     vy = std::clamp(vel.y() / 50.0f, -1.5f, 1.5f);
            float     vz = std::clamp(vel.z() / 50.0f, -1.0f, 1.0f);
            pVeh->SetMoveSpeed(CVector(vx, vy, vz));
        }
        else
        {
            pEntity->SetMatrix(mat);
        }
    }
}
