/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef BT_SEQUENTIAL_IMPULSE_CONSTRAINT_SOLVER_H
#define BT_SEQUENTIAL_IMPULSE_CONSTRAINT_SOLVER_H

class btIDebugDraw;
class btPersistentManifold;
class btDispatcher;
class btCollisionObject;
#include "BulletDynamics/ConstraintSolver/btTypedConstraint.h"
#include "BulletDynamics/ConstraintSolver/btContactSolverInfo.h"
#include "BulletDynamics/ConstraintSolver/btSolverBody.h"
#include "BulletDynamics/ConstraintSolver/btSolverConstraint.h"
#include "BulletCollision/NarrowPhaseCollision/btManifoldPoint.h"
#include "BulletDynamics/ConstraintSolver/btConstraintSolver.h"

typedef btScalar (*btSingleConstraintRowSolver)(btSolverBody&, btSolverBody&, const btSolverConstraint&);

struct btSISolverSingleIterationData
{
	btAlignedObjectArray<btSolverBody>& m_tmpSolverBodyPool;
	btConstraintArray& m_tmpSolverContactConstraintPool;
	btConstraintArray& m_tmpSolverNonContactConstraintPool;
	btConstraintArray& m_tmpSolverContactFrictionConstraintPool;
	btConstraintArray& m_tmpSolverContactRollingFrictionConstraintPool;

	btAlignedObjectArray<int>& m_orderTmpConstraintPool;
	btAlignedObjectArray<int>& m_orderNonContactConstraintPool;
	btAlignedObjectArray<int>& m_orderFrictionConstraintPool;
	btAlignedObjectArray<btTypedConstraint::btConstraintInfo1>& m_tmpConstraintSizesPool;
	unsigned long& m_seed;

	btSingleConstraintRowSolver& m_resolveSingleConstraintRowGeneric;
	btSingleConstraintRowSolver& m_resolveSingleConstraintRowLowerLimit;
	btSingleConstraintRowSolver& m_resolveSplitPenetrationImpulse;
	btAlignedObjectArray<int>& m_kinematicBodyUniqueIdToSolverBodyTable;
	int& m_fixedBodyId;
	int& m_maxOverrideNumSolverIterations;
	int getOrInitSolverBody(btCollisionObject & body, btScalar timeStep);
	static void initSolverBody(btSolverBody * solverBody, btCollisionObject * collisionObject, btScalar timeStep);
	int getSolverBody(btCollisionObject& body) const;


	btSISolverSingleIterationData(btAlignedObjectArray<btSolverBody>& tmpSolverBodyPool,
		btConstraintArray& tmpSolverContactConstraintPool,
		btConstraintArray& tmpSolverNonContactConstraintPool,
		btConstraintArray& tmpSolverContactFrictionConstraintPool,
		btConstraintArray& tmpSolverContactRollingFrictionConstraintPool,
		btAlignedObjectArray<int>& orderTmpConstraintPool,
		btAlignedObjectArray<int>& orderNonContactConstraintPool,
		btAlignedObjectArray<int>& orderFrictionConstraintPool,
		btAlignedObjectArray<btTypedConstraint::btConstraintInfo1>& tmpConstraintSizesPool,
		btSingleConstraintRowSolver& resolveSingleConstraintRowGeneric,
		btSingleConstraintRowSolver& resolveSingleConstraintRowLowerLimit,
		btSingleConstraintRowSolver& resolveSplitPenetrationImpulse,
		btAlignedObjectArray<int>& kinematicBodyUniqueIdToSolverBodyTable,
		unsigned long& seed,
		int& fixedBodyId,
		int& maxOverrideNumSolverIterations
	)
		:m_tmpSolverBodyPool(tmpSolverBodyPool),
		m_tmpSolverContactConstraintPool(tmpSolverContactConstraintPool),
		m_tmpSolverNonContactConstraintPool(tmpSolverNonContactConstraintPool),
		m_tmpSolverContactFrictionConstraintPool(tmpSolverContactFrictionConstraintPool),
		m_tmpSolverContactRollingFrictionConstraintPool(tmpSolverContactRollingFrictionConstraintPool),
		m_orderTmpConstraintPool(orderTmpConstraintPool),
		m_orderNonContactConstraintPool(orderNonContactConstraintPool),
		m_orderFrictionConstraintPool(orderFrictionConstraintPool),
		m_tmpConstraintSizesPool(tmpConstraintSizesPool),
		m_resolveSingleConstraintRowGeneric(resolveSingleConstraintRowGeneric),
		m_resolveSingleConstraintRowLowerLimit(resolveSingleConstraintRowLowerLimit),
		m_resolveSplitPenetrationImpulse(resolveSplitPenetrationImpulse),
		m_kinematicBodyUniqueIdToSolverBodyTable(kinematicBodyUniqueIdToSolverBodyTable),
		m_seed(seed),
		m_fixedBodyId(fixedBodyId),
		m_maxOverrideNumSolverIterations(maxOverrideNumSolverIterations)
	{
	}
};

///The btSequentialImpulseConstraintSolver is a fast SIMD implementation of the Projected Gauss Seidel (iterative LCP) method.
ATTRIBUTE_ALIGNED16(class)
btSequentialImpulseConstraintSolver : public btConstraintSolver
{
protected:
	btAlignedObjectArray<btSolverBody> m_tmpSolverBodyPool;
	btConstraintArray m_tmpSolverContactConstraintPool;
	btConstraintArray m_tmpSolverNonContactConstraintPool;
	btConstraintArray m_tmpSolverContactFrictionConstraintPool;
	btConstraintArray m_tmpSolverContactRollingFrictionConstraintPool;

	btAlignedObjectArray<int> m_orderTmpConstraintPool;
	btAlignedObjectArray<int> m_orderNonContactConstraintPool;
	btAlignedObjectArray<int> m_orderFrictionConstraintPool;
	btAlignedObjectArray<btTypedConstraint::btConstraintInfo1> m_tmpConstraintSizesPool;
	int m_maxOverrideNumSolverIterations;
	int m_fixedBodyId;
	// When running solvers on multiple threads, a race condition exists for Kinematic objects that
	// participate in more than one solver.
	// The getOrInitSolverBody() function writes the companionId of each body (storing the index of the solver body
	// for the current solver). For normal dynamic bodies it isn't an issue because they can only be in one island
	// (and therefore one thread) at a time. But kinematic bodies can be in multiple islands at once.
	// To avoid this race condition, this solver does not write the companionId, instead it stores the solver body
	// index in this solver-local table, indexed by the uniqueId of the body.
	btAlignedObjectArray<int> m_kinematicBodyUniqueIdToSolverBodyTable;  // only used for multithreading

	btSingleConstraintRowSolver m_resolveSingleConstraintRowGeneric;
	btSingleConstraintRowSolver m_resolveSingleConstraintRowLowerLimit;
	btSingleConstraintRowSolver m_resolveSplitPenetrationImpulse;
	int m_cachedSolverMode;  // used to check if SOLVER_SIMD flag has been changed
	void setupSolverFunctions(bool useSimd);

	btScalar m_leastSquaresResidual;

	void setupFrictionConstraint(btSolverConstraint & solverConstraint, const btVector3& normalAxis, int solverBodyIdA, int solverBodyIdB,
		btManifoldPoint& cp, const btVector3& rel_pos1, const btVector3& rel_pos2,
		btCollisionObject* colObj0, btCollisionObject* colObj1, btScalar relaxation,
		const btContactSolverInfo& infoGlobal,
		btScalar desiredVelocity = 0., btScalar cfmSlip = 0.);

	void setupTorsionalFrictionConstraint(btSolverConstraint & solverConstraint, const btVector3& normalAxis, int solverBodyIdA, int solverBodyIdB,
		btManifoldPoint& cp, btScalar combinedTorsionalFriction, const btVector3& rel_pos1, const btVector3& rel_pos2,
		btCollisionObject* colObj0, btCollisionObject* colObj1, btScalar relaxation,
		btScalar desiredVelocity = 0., btScalar cfmSlip = 0.);

	btSolverConstraint& addFrictionConstraint(const btVector3& normalAxis, int solverBodyIdA, int solverBodyIdB, int frictionIndex, btManifoldPoint& cp, const btVector3& rel_pos1, const btVector3& rel_pos2, btCollisionObject* colObj0, btCollisionObject* colObj1, btScalar relaxation, const btContactSolverInfo& infoGlobal, btScalar desiredVelocity = 0., btScalar cfmSlip = 0.);
	btSolverConstraint& addTorsionalFrictionConstraint(const btVector3& normalAxis, int solverBodyIdA, int solverBodyIdB, int frictionIndex, btManifoldPoint& cp, btScalar torsionalFriction, const btVector3& rel_pos1, const btVector3& rel_pos2, btCollisionObject* colObj0, btCollisionObject* colObj1, btScalar relaxation, btScalar desiredVelocity = 0, btScalar cfmSlip = 0.f);

	void setupContactConstraint(btSolverConstraint & solverConstraint, int solverBodyIdA, int solverBodyIdB, btManifoldPoint& cp,
		const btContactSolverInfo& infoGlobal, btScalar& relaxation, const btVector3& rel_pos1, const btVector3& rel_pos2);

	static void applyAnisotropicFriction(btCollisionObject * colObj, btVector3 & frictionDirection, int frictionMode);

	void setFrictionConstraintImpulse(btSolverConstraint & solverConstraint, int solverBodyIdA, int solverBodyIdB,
		btManifoldPoint& cp, const btContactSolverInfo& infoGlobal);

	///m_btSeed2 is used for re-arranging the constraint rows. improves convergence/quality of friction
	unsigned long m_btSeed2;

	btScalar restitutionCurve(btScalar rel_vel, btScalar restitution, btScalar velocityThreshold);

	virtual void convertContacts(btPersistentManifold * *manifoldPtr, int numManifolds, const btContactSolverInfo& infoGlobal);

	void convertContact(btPersistentManifold * manifold, const btContactSolverInfo& infoGlobal);

	virtual void convertJoints(btTypedConstraint * *constraints, int numConstraints, const btContactSolverInfo& infoGlobal);
	void convertJoint(btSolverConstraint * currentConstraintRow, btTypedConstraint * constraint, const btTypedConstraint::btConstraintInfo1& info1, int solverBodyIdA, int solverBodyIdB, const btContactSolverInfo& infoGlobal);


	virtual void convertBodies(btCollisionObject * *bodies, int numBodies, const btContactSolverInfo& infoGlobal);

	btScalar resolveSplitPenetrationSIMD(btSolverBody & bodyA, btSolverBody & bodyB, const btSolverConstraint& contactConstraint)
	{
		return m_resolveSplitPenetrationImpulse(bodyA, bodyB, contactConstraint);
	}

	btScalar resolveSplitPenetrationImpulseCacheFriendly(btSolverBody & bodyA, btSolverBody & bodyB, const btSolverConstraint& contactConstraint)
	{
		return m_resolveSplitPenetrationImpulse(bodyA, bodyB, contactConstraint);
	}

	//internal method
	int getOrInitSolverBody(btCollisionObject & body, btScalar timeStep);
	void initSolverBody(btSolverBody * solverBody, btCollisionObject * collisionObject, btScalar timeStep);

	btScalar resolveSingleConstraintRowGeneric(btSolverBody & bodyA, btSolverBody & bodyB, const btSolverConstraint& contactConstraint);
	btScalar resolveSingleConstraintRowGenericSIMD(btSolverBody & bodyA, btSolverBody & bodyB, const btSolverConstraint& contactConstraint);
	btScalar resolveSingleConstraintRowLowerLimit(btSolverBody & bodyA, btSolverBody & bodyB, const btSolverConstraint& contactConstraint);
	btScalar resolveSingleConstraintRowLowerLimitSIMD(btSolverBody & bodyA, btSolverBody & bodyB, const btSolverConstraint& contactConstraint);
	btScalar resolveSplitPenetrationImpulse(btSolverBody & bodyA, btSolverBody & bodyB, const btSolverConstraint& contactConstraint)
	{
		return m_resolveSplitPenetrationImpulse(bodyA, bodyB, contactConstraint);
	}

public:

	void writeBackContacts(int iBegin, int iEnd, const btContactSolverInfo& infoGlobal);
	void writeBackJoints(int iBegin, int iEnd, const btContactSolverInfo& infoGlobal);
	void writeBackBodies(int iBegin, int iEnd, const btContactSolverInfo& infoGlobal);
	virtual void solveGroupCacheFriendlySplitImpulseIterations(btCollisionObject * *bodies, int numBodies, btPersistentManifold** manifoldPtr, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& infoGlobal, btIDebugDraw* debugDrawer);
	virtual btScalar solveGroupCacheFriendlyFinish(btCollisionObject * *bodies, int numBodies, const btContactSolverInfo& infoGlobal);
	virtual btScalar solveSingleIteration(int iteration, btCollisionObject** bodies, int numBodies, btPersistentManifold** manifoldPtr, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& infoGlobal, btIDebugDraw* debugDrawer);


	virtual btScalar solveGroupCacheFriendlySetup(btCollisionObject * *bodies, int numBodies, btPersistentManifold** manifoldPtr, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& infoGlobal, btIDebugDraw* debugDrawer);
	virtual btScalar solveGroupCacheFriendlyIterations(btCollisionObject * *bodies, int numBodies, btPersistentManifold** manifoldPtr, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& infoGlobal, btIDebugDraw* debugDrawer);

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btSequentialImpulseConstraintSolver();
	virtual ~btSequentialImpulseConstraintSolver();

	virtual btScalar solveGroup(btCollisionObject * *bodies, int numBodies, btPersistentManifold** manifold, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& info, btIDebugDraw* debugDrawer, btDispatcher* dispatcher);

	static btScalar solveSingleIterationInternal(btSISolverSingleIterationData& siData, int iteration, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& infoGlobal);
	static void convertBodiesInternal(btSISolverSingleIterationData& siData, btCollisionObject** bodies, int numBodies, const btContactSolverInfo& infoGlobal);
	static void convertJointsInternal(btSISolverSingleIterationData& siData, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& infoGlobal);
	static void convertContactInternal(btSISolverSingleIterationData& siData, btPersistentManifold * manifold, const btContactSolverInfo& infoGlobal);
	static void setupContactConstraintInternal(btSISolverSingleIterationData& siData, btSolverConstraint& solverConstraint, int solverBodyIdA, int solverBodyIdB, btManifoldPoint& cp, const btContactSolverInfo& infoGlobal, btScalar& relaxation,
		const btVector3& rel_pos1, const btVector3& rel_pos2);
	static btScalar restitutionCurveInternal(btScalar rel_vel, btScalar restitution, btScalar velocityThreshold);
	static btSolverConstraint& addTorsionalFrictionConstraintInternal(btAlignedObjectArray<btSolverBody>& tmpSolverBodyPool, btConstraintArray& tmpSolverContactRollingFrictionConstraintPool, const btVector3& normalAxis, int solverBodyIdA, int solverBodyIdB, int frictionIndex, btManifoldPoint& cp, btScalar combinedTorsionalFriction, const btVector3& rel_pos1, const btVector3& rel_pos2, btCollisionObject* colObj0, btCollisionObject* colObj1, btScalar relaxation, btScalar desiredVelocity = 0, btScalar cfmSlip = 0.);
	static void setupTorsionalFrictionConstraintInternal(btAlignedObjectArray<btSolverBody>& tmpSolverBodyPool, btSolverConstraint& solverConstraint, const btVector3& normalAxis1, int solverBodyIdA, int solverBodyIdB,
		btManifoldPoint& cp, btScalar combinedTorsionalFriction, const btVector3& rel_pos1, const btVector3& rel_pos2,
		btCollisionObject* colObj0, btCollisionObject* colObj1, btScalar relaxation,
		btScalar desiredVelocity, btScalar cfmSlip);
	static void setupFrictionConstraintInternal(btAlignedObjectArray<btSolverBody>& tmpSolverBodyPool, btSolverConstraint& solverConstraint, const btVector3& normalAxis, int solverBodyIdA, int solverBodyIdB, btManifoldPoint& cp, const btVector3& rel_pos1, const btVector3& rel_pos2, btCollisionObject* colObj0, btCollisionObject* colObj1, btScalar relaxation, const btContactSolverInfo& infoGlobal, btScalar desiredVelocity, btScalar cfmSlip);
	static btSolverConstraint& addFrictionConstraintInternal(btAlignedObjectArray<btSolverBody>& tmpSolverBodyPool, btConstraintArray& tmpSolverContactFrictionConstraintPool, const btVector3& normalAxis, int solverBodyIdA, int solverBodyIdB, int frictionIndex, btManifoldPoint& cp, const btVector3& rel_pos1, const btVector3& rel_pos2, btCollisionObject* colObj0, btCollisionObject* colObj1, btScalar relaxation, const btContactSolverInfo& infoGlobal, btScalar desiredVelocity = 0., btScalar cfmSlip = 0.);
	static void setFrictionConstraintImpulseInternal(btAlignedObjectArray<btSolverBody>& tmpSolverBodyPool, btConstraintArray& tmpSolverContactFrictionConstraintPool,

		btSolverConstraint& solverConstraint,
		int solverBodyIdA, int solverBodyIdB,
		btManifoldPoint& cp, const btContactSolverInfo& infoGlobal);
	static void convertJointInternal(btAlignedObjectArray<btSolverBody>& tmpSolverBodyPool,
		int& maxOverrideNumSolverIterations,
		btSolverConstraint* currentConstraintRow,
		btTypedConstraint* constraint,
		const btTypedConstraint::btConstraintInfo1& info1,
		int solverBodyIdA,
		int solverBodyIdB,
		const btContactSolverInfo& infoGlobal);

	static btScalar solveGroupCacheFriendlyFinishInternal(btSISolverSingleIterationData& siData, btCollisionObject** bodies, int numBodies, const btContactSolverInfo& infoGlobal);

	static void writeBackContactsInternal(btConstraintArray& tmpSolverContactConstraintPool, btConstraintArray& tmpSolverContactFrictionConstraintPool, int iBegin, int iEnd, const btContactSolverInfo& infoGlobal);

	static void writeBackJointsInternal(btConstraintArray& tmpSolverNonContactConstraintPool, int iBegin, int iEnd, const btContactSolverInfo& infoGlobal);
	static void writeBackBodiesInternal(btAlignedObjectArray<btSolverBody>& tmpSolverBodyPool, int iBegin, int iEnd, const btContactSolverInfo& infoGlobal);
	static void solveGroupCacheFriendlySplitImpulseIterationsInternal(btSISolverSingleIterationData& siData, btCollisionObject** bodies, int numBodies, btPersistentManifold** manifoldPtr, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& infoGlobal, btIDebugDraw* debugDrawer);


	///clear internal cached data and reset random seed
	virtual void reset();

	unsigned long btRand2();
	int btRandInt2(int n);

	static unsigned long btRand2a(unsigned long& seed);
	static int btRandInt2a(int n, unsigned long& seed);

	void setRandSeed(unsigned long seed)
	{
		m_btSeed2 = seed;
	}
	unsigned long getRandSeed() const
	{
		return m_btSeed2;
	}

	virtual btConstraintSolverType getSolverType() const
	{
		return BT_SEQUENTIAL_IMPULSE_SOLVER;
	}

	btSingleConstraintRowSolver getActiveConstraintRowSolverGeneric()
	{
		return m_resolveSingleConstraintRowGeneric;
	}
	void setConstraintRowSolverGeneric(btSingleConstraintRowSolver rowSolver)
	{
		m_resolveSingleConstraintRowGeneric = rowSolver;
	}
	btSingleConstraintRowSolver getActiveConstraintRowSolverLowerLimit()
	{
		return m_resolveSingleConstraintRowLowerLimit;
	}
	void setConstraintRowSolverLowerLimit(btSingleConstraintRowSolver rowSolver)
	{
		m_resolveSingleConstraintRowLowerLimit = rowSolver;
	}

	///Various implementations of solving a single constraint row using a generic equality constraint, using scalar reference, SSE2 or SSE4
	static btSingleConstraintRowSolver getScalarConstraintRowSolverGeneric();
	static btSingleConstraintRowSolver getSSE2ConstraintRowSolverGeneric();
	static btSingleConstraintRowSolver getSSE4_1ConstraintRowSolverGeneric();

	///Various implementations of solving a single constraint row using an inequality (lower limit) constraint, using scalar reference, SSE2 or SSE4
	static btSingleConstraintRowSolver getScalarConstraintRowSolverLowerLimit();
	static btSingleConstraintRowSolver getSSE2ConstraintRowSolverLowerLimit();
	static btSingleConstraintRowSolver getSSE4_1ConstraintRowSolverLowerLimit();

	static btSingleConstraintRowSolver getScalarSplitPenetrationImpulseGeneric();
	static btSingleConstraintRowSolver getSSE2SplitPenetrationImpulseGeneric();

};

#endif  //BT_SEQUENTIAL_IMPULSE_CONSTRAINT_SOLVER_H