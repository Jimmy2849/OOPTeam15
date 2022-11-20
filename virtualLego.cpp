////////////////////////////////////////////////////////////////////////////////
// 
// File: virtualLego.cpp
//
// Original Author: ��â�� Chang-hyeon Park, 
// Modified by Bong-Soo Sohn and Dong-Jun Kim
// 
// Originally programmed for Virtual LEGO. 
// Modified later to program for Virtual Billiard.
//        
////////////////////////////////////////////////////////////////////////////////
// curling , player ��ü �߰�
// setup���� �� �����κ��� Curling ��ü ������ ����, Curing��ü game�� �������� ����, display������ �̿� �°� ��ü, VK_SPACE �κ� g_sphere[i] �� game.getPlayer().getball(i)�� ��ü.
// display �Լ����� game �� now_turn �� Ȯ���Ͽ� ���� ����Ǿ�����( �����̽��ٸ� ���� now_turn++ �� �ǰ� ���� ��� ���߾�����) �׿� �°� �����Ѵ�.
// ���� ��� ���� ����Ǿ����� game.nextSet()�� ȣ���Ͽ� ���� ��Ʈ�� �����Ѵ�.
// CSphere �� bool isPlaying, void setIsPlaying() �߰�. �� ���� player �� ������ ���ӿ� ������ ������ �Ǵ�. 
// ���� 1p ���� �����ϰ� �״��� 2p �� ���� ������. Curling.whose_trun �� ���� player�� ��Ÿ����, ���������� ���� �ٲ��.
// **���ӿ� �������� ���� ��������� ������ ��������..
// ���� ��Ʈ�� �Ѿ�� ������ Curling.total_score�� �ջ�. �̸� ��ܿ� ��� ( �ؽ�Ʈ ������ ���� ���� )
// ��� ��Ʈ�� ������ ���� ��� ���. ���� ���� ���� x
// ��¥ �ø�ó�� ���� ��� - ���� ����� ������?
// ��Ʈ���� �� �÷��̾� ��ü - ¦�� ��Ʈ �� 2�÷��̾� ����

#include "d3dUtility.h"
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <string>
#include "CText.h"
#include <algorithm> // Culring class���� ���� ���� sort ���

IDirect3DDevice9* Device = NULL;

// window size
const int Width = 1024;
const int Height = 768;

// There are four balls
// initialize the position (coordinate) of each ball (ball0 ~ ball3)
const float spherePos[4][2] = { {-2.8f,-4.2f} , {-2.3f,-4.2f} , {-1.8f, -4.2f} , {-1.3f,-4.2f} }; // �ʱ� ������ ��ġ
const float spherePos2[4][2] = { {+2.8f,-4.2f} , {+2.3f,-4.2f} , {+1.8f, -4.2f} , {+1.3f,-4.2f} }; // �ʱ� ����� ��ġ
// initialize the color of each ball (ball0 ~ ball3)
const D3DXCOLOR sphereColor[4] = { d3d::RED, d3d::RED, d3d::YELLOW, d3d::WHITE };

// -----------------------------------------------------------------------------
// Transform matrices
// -----------------------------------------------------------------------------
D3DXMATRIX g_mWorld;
D3DXMATRIX g_mView;
D3DXMATRIX g_mProj;


#define M_RADIUS 0.21   // ball radius
#define PI 3.14159265
#define M_HEIGHT 0.01
#define DECREASE_RATE 0.9982
#define TIME_LIMIT 10

D3DXVECTOR3 scoreTarget = { 0.0f, (float)M_RADIUS, 0.0f }; // ���� ������ �Ǵ� ������ (ȭ��� �Ͼ� �� ���̴� �߾�)

// -----------------------------------------------------------------------------
// CSphere class definition
// -----------------------------------------------------------------------------

class CSphere {
private:
	float					center_x, center_y, center_z;
	float					pre_center_x, pre_center_z; // �浹 �� �� ��ǥ
	float                   m_radius;
	float					m_velocity_x;
	float					m_velocity_z;
	bool					isPlaying;
	float					distance;

public:
	CSphere(void)
	{
		D3DXMatrixIdentity(&m_mLocal);
		ZeroMemory(&m_mtrl, sizeof(m_mtrl));
		m_radius = 0;
		m_velocity_x = 0;
		m_velocity_z = 0;
		isPlaying = false;
		distance = 99;
		m_pSphereMesh = NULL;
	}
	~CSphere(void) {}

public:
	bool create(IDirect3DDevice9* pDevice, D3DXCOLOR color = d3d::WHITE)
	{
		if (NULL == pDevice)
			return false;

		m_mtrl.Ambient = color;
		m_mtrl.Diffuse = color;
		m_mtrl.Specular = color;
		m_mtrl.Emissive = d3d::BLACK;
		m_mtrl.Power = 5.0f;

		if (FAILED(D3DXCreateSphere(pDevice, getRadius(), 50, 50, &m_pSphereMesh, NULL)))
			return false;
		return true;
	}

	void destroy(void)
	{
		if (m_pSphereMesh != NULL) {
			m_pSphereMesh->Release();
			m_pSphereMesh = NULL;
		}
	}

	void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
	{
		if (NULL == pDevice)
			return;
		pDevice->SetTransform(D3DTS_WORLD, &mWorld);
		pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
		pDevice->SetMaterial(&m_mtrl);
		m_pSphereMesh->DrawSubset(0);
	}

	void setDistance(void) {
		distance = sqrt(pow((scoreTarget.x - this->center_x), 2) + pow((scoreTarget.z - this->center_z), 2));
	}

	bool isStop(void) // �� ���� ����
	{
		if (this->m_velocity_x == 0 && this->m_velocity_z == 0) {
			return true;
		}
		else {
			return false;
		}
	}
	bool hasIntersected(CSphere& ball) // �� �浹 ����
	{
		D3DXVECTOR3 b1 = this->getCenter();
		D3DXVECTOR3 b2 = ball.getCenter();

		double distance = sqrt(pow((b1.x - b2.x), 2) + pow((b1.y - b2.y), 2) + pow((b1.z - b2.z), 2)); // ���� �߽� ��ǥ�� ���� ���� �� �� ������ �Ÿ�
		double rSum = this->getRadius() + ball.getRadius(); // �� ���� �������� ��

		// �� �� ������ �Ÿ��� �� ���� �������� �պ��� �۰ų� ������ �� ���� �浹�� �� && �� ���� isPlaying�� ��� true���� �浹 ������ ����
		if (distance <= rSum && this->isPlaying && ball.isPlaying) {
			return true;
		}
		else {
			return false;
		}

	}

	void fixPosition(CSphere& ball) // �� ��ġ�� �ʵ��� �浹 ���� ��ǥ�� �̵���Ű�� �Լ�
	{
		D3DXVECTOR3 temp = ball.getCenter();

		// �� �浹 ��, �浹 ������ ��ǥ�� �ٻ��ϰ� �̵�
		this->setCenter((this->center_x + this->pre_center_x) / 2, this->center_y, (this->center_z + this->pre_center_z) / 2);
		ball.setCenter((temp.x + ball.pre_center_x) / 2, temp.y, (temp.z + ball.pre_center_z) / 2);

		this->setCenter(this->pre_center_x, this->center_y, this->pre_center_z);
		ball.setCenter(ball.pre_center_x, temp.y, ball.pre_center_z);

	}

	void hitBy(CSphere& ball) // �� �浹 ó��
	{
		/* ���� �� ���� : http://egloos.zum.com/hhugs/v/3506086 */

		if (hasIntersected(ball)) {

			fixPosition(ball); // �� ��ġ�� �ʵ��� ��ġ ����

			double x = this->center_x - ball.center_x; // �� �� ������ x��ǥ �Ÿ�
			double z = this->center_z - ball.center_z; // �� �� ������ z��ǥ �Ÿ�
			double distance = sqrt((x * x) + (z * z)); // �� �� ������ ���� �Ÿ�

			// this�� �浹 �� �ӵ�
			double vax = this->m_velocity_x;
			double vaz = this->m_velocity_z;

			// ball�� �浹 �� �ӵ�
			double vbx = ball.m_velocity_x;
			double vbz = ball.m_velocity_z;

			// �� = ���� ����� �� ���� �߽��� ���� ������ �̷�� ��
			double cos_t = x / distance; // cos ��
			double sin_t = z / distance; // sin ��

			// �浹 �� �� ���� x'���� �ӵ�
			double vaxp = vbx * cos_t + vbz * sin_t;
			double vbxp = vax * cos_t + vaz * sin_t;

			// �浹 �� �� ���� z'���� �ӵ�
			double vazp = vaz * cos_t - vax * sin_t;
			double vbzp = vbz * cos_t - vbx * sin_t;

			// �浹 �� this�� �ӵ�
			double vaxp2 = vaxp * cos_t - vazp * sin_t;
			double vazp2 = vaxp * sin_t + vazp * cos_t;

			// �浹 �� ball�� �ӵ�
			double vbxp2 = vbxp * cos_t - vbzp * sin_t;
			double vbzp2 = vbxp * sin_t + vbzp * cos_t;

			this->setPower(vaxp2, vazp2);
			ball.setPower(vbxp2, vbzp2);

		}
	}

	void ballUpdate(float timeDiff)
	{
		const float TIME_SCALE = 3.3;
		D3DXVECTOR3 cord = this->getCenter();
		double vx = abs(this->getVelocity_X());
		double vz = abs(this->getVelocity_Z());

		this->pre_center_x = cord.x;
		this->pre_center_z = cord.z;

		if (vx > 0.01 || vz > 0.01)
		{
			float tX = cord.x + TIME_SCALE * timeDiff * m_velocity_x;
			float tZ = cord.z + TIME_SCALE * timeDiff * m_velocity_z;

			this->setCenter(tX, cord.y, tZ);
		}
		else { this->setPower(0, 0); }
		//this->setPower(this->getVelocity_X() * DECREASE_RATE, this->getVelocity_Z() * DECREASE_RATE);
		double rate = 1 - (1 - DECREASE_RATE) * timeDiff * 400;
		if (rate < 0)
			rate = 0;
		this->setPower(getVelocity_X() * rate, getVelocity_Z() * rate);
	}

	double getVelocity_X() { return this->m_velocity_x; }
	double getVelocity_Z() { return this->m_velocity_z; }

	void setPower(double vx, double vz)
	{
		this->m_velocity_x = vx;
		this->m_velocity_z = vz;
	}

	void setCenter(float x, float y, float z)
	{
		D3DXMATRIX m;
		center_x = x;	center_y = y;	center_z = z;
		D3DXMatrixTranslation(&m, x, y, z);
		setLocalTransform(m);
	}

	float getRadius(void)  const { return (float)(M_RADIUS); }
	const D3DXMATRIX& getLocalTransform(void) const { return m_mLocal; }
	void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }
	D3DXVECTOR3 getCenter(void) const
	{
		D3DXVECTOR3 org(center_x, center_y, center_z);
		return org;
	}
	float getDistance() { return distance; }
	bool getIsPlaying() {
		return isPlaying;
	}
	void setIsPlaying(bool b) {
		isPlaying = b;
	}

private:
	D3DXMATRIX              m_mLocal;
	D3DMATERIAL9            m_mtrl;
	ID3DXMesh* m_pSphereMesh;

};



// -----------------------------------------------------------------------------
// CWall class definition
// -----------------------------------------------------------------------------

class CWall {

private:

	float					m_x;
	float					m_z;
	float                   m_width;
	float                   m_depth;
	float					m_height;

public:
	CWall(void)
	{
		D3DXMatrixIdentity(&m_mLocal);
		ZeroMemory(&m_mtrl, sizeof(m_mtrl));
		m_width = 0;
		m_depth = 0;
		m_pBoundMesh = NULL;
	}
	~CWall(void) {}
public:
	bool create(IDirect3DDevice9* pDevice, float ix, float iz, float iwidth, float iheight, float idepth, D3DXCOLOR color = d3d::WHITE)
	{
		if (NULL == pDevice)
			return false;

		m_mtrl.Ambient = color;
		m_mtrl.Diffuse = color;
		m_mtrl.Specular = color;
		m_mtrl.Emissive = d3d::BLACK;
		m_mtrl.Power = 5.0f;

		m_width = iwidth;
		m_depth = idepth;

		if (FAILED(D3DXCreateBox(pDevice, iwidth, iheight, idepth, &m_pBoundMesh, NULL)))
			return false;
		return true;
	}
	void destroy(void)
	{
		if (m_pBoundMesh != NULL) {
			m_pBoundMesh->Release();
			m_pBoundMesh = NULL;
		}
	}
	void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
	{
		if (NULL == pDevice)
			return;
		pDevice->SetTransform(D3DTS_WORLD, &mWorld);
		pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
		pDevice->SetMaterial(&m_mtrl);
		m_pBoundMesh->DrawSubset(0);
	}

	bool hasIntersected(CSphere& ball)
	{
		float ball_x = ball.getCenter().x;
		float ball_z = ball.getCenter().z;

		// ���� x or z��ǥ�� ���� ��ǥ�� ���ų� �Ѿ ��� �浹 ���� 
		if ((ball_x >= (3.0 - M_RADIUS)) || (ball_x <= (-3.0 + M_RADIUS)) || (ball_z <= (-4.5 + M_RADIUS)) || (ball_z >= (4.5 - M_RADIUS))) {
			return true;
		}
		else {
			return false;
		}
	}

	void hitBy(CSphere& ball)
	{
		if (hasIntersected(ball)) {

			// ���� ���� �浹���� ���
			if (ball.getCenter().x <= (-3.0 + M_RADIUS)) {
				ball.setCenter(-3.0 + M_RADIUS, ball.getCenter().y, ball.getCenter().z); // �� ��ǥ �ٽ� ����
				ball.setPower((-1) * ball.getVelocity_X(), ball.getVelocity_Z()); // x���� �ӵ��� �ݴ� �������� �ٲ�
				ball.setPower(ball.getVelocity_X() * 0.8f, ball.getVelocity_Z() * 0.8f); // �� �ε��� �� ���� ó��
			}

			// ������ ���� �浹���� ���
			if (ball.getCenter().x >= (3.0 - M_RADIUS)) {
				ball.setCenter(3.0 - M_RADIUS, ball.getCenter().y, ball.getCenter().z); // �� ��ǥ �ٽ� ����
				ball.setPower((-1) * ball.getVelocity_X(), ball.getVelocity_Z()); // x���� �ӵ��� �ݴ� �������� �ٲ�
				ball.setPower(ball.getVelocity_X() * 0.8f, ball.getVelocity_Z() * 0.8f); // �� �ε��� �� ���� ó��
			}

			// ���� ���� �浹���� ���
			if (ball.getCenter().z >= (4.5 - M_RADIUS)) {
				ball.setCenter(ball.getCenter().x, ball.getCenter().y, 4.5 - M_RADIUS); // �� ��ǥ �ٽ� ����
				ball.setPower(ball.getVelocity_X(), (-1) * ball.getVelocity_Z()); // z���� �ӵ��� �ݴ� �������� �ٲ�
				ball.setPower(ball.getVelocity_X() * 0.8f, ball.getVelocity_Z() * 0.8f); // �� �ε��� �� ���� ó��
			}

			// �Ʒ��� ���� �浹���� ���
			if (ball.getCenter().z <= (-4.5 + M_RADIUS)) {
				ball.setCenter(ball.getCenter().x, ball.getCenter().y, -4.5 + M_RADIUS); // �� ��ǥ �ٽ� ����
				ball.setPower(ball.getVelocity_X(), (-1) * ball.getVelocity_Z()); // z���� �ӵ��� �ݴ� �������� �ٲ�
				ball.setPower(ball.getVelocity_X() * 0.8f, ball.getVelocity_Z() * 0.8f); // �� �ε��� �� ���� ó��
			}
		}

	}

	void setPosition(float x, float y, float z)
	{
		D3DXMATRIX m;
		this->m_x = x;
		this->m_z = z;

		D3DXMatrixTranslation(&m, x, y, z);
		setLocalTransform(m);
	}

	float getHeight(void) const { return M_HEIGHT; }



private:
	void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }

	D3DXMATRIX              m_mLocal;
	D3DMATERIAL9            m_mtrl;
	ID3DXMesh* m_pBoundMesh;
};

// -----------------------------------------------------------------------------
// CLight class definition
// -----------------------------------------------------------------------------

class CLight {
public:
	CLight(void)
	{
		static DWORD i = 0;
		m_index = i++;
		D3DXMatrixIdentity(&m_mLocal);
		::ZeroMemory(&m_lit, sizeof(m_lit));
		m_pMesh = NULL;
		m_bound._center = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
		m_bound._radius = 0.0f;
	}
	~CLight(void) {}
public:
	bool create(IDirect3DDevice9* pDevice, const D3DLIGHT9& lit, float radius = 0.1f)
	{
		if (NULL == pDevice)
			return false;
		if (FAILED(D3DXCreateSphere(pDevice, radius, 10, 10, &m_pMesh, NULL)))
			return false;

		m_bound._center = lit.Position;
		m_bound._radius = radius;

		m_lit.Type = lit.Type;
		m_lit.Diffuse = lit.Diffuse;
		m_lit.Specular = lit.Specular;
		m_lit.Ambient = lit.Ambient;
		m_lit.Position = lit.Position;
		m_lit.Direction = lit.Direction;
		m_lit.Range = lit.Range;
		m_lit.Falloff = lit.Falloff;
		m_lit.Attenuation0 = lit.Attenuation0;
		m_lit.Attenuation1 = lit.Attenuation1;
		m_lit.Attenuation2 = lit.Attenuation2;
		m_lit.Theta = lit.Theta;
		m_lit.Phi = lit.Phi;
		return true;
	}
	void destroy(void)
	{
		if (m_pMesh != NULL) {
			m_pMesh->Release();
			m_pMesh = NULL;
		}
	}
	bool setLight(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
	{
		if (NULL == pDevice)
			return false;

		D3DXVECTOR3 pos(m_bound._center);
		D3DXVec3TransformCoord(&pos, &pos, &m_mLocal);
		D3DXVec3TransformCoord(&pos, &pos, &mWorld);
		m_lit.Position = pos;

		pDevice->SetLight(m_index, &m_lit);
		pDevice->LightEnable(m_index, TRUE);
		return true;
	}

	void draw(IDirect3DDevice9* pDevice)
	{
		if (NULL == pDevice)
			return;
		D3DXMATRIX m;
		D3DXMatrixTranslation(&m, m_lit.Position.x, m_lit.Position.y, m_lit.Position.z);
		pDevice->SetTransform(D3DTS_WORLD, &m);
		pDevice->SetMaterial(&d3d::WHITE_MTRL);
		m_pMesh->DrawSubset(0);
	}

	D3DXVECTOR3 getPosition(void) const { return D3DXVECTOR3(m_lit.Position); }

private:
	DWORD               m_index;
	D3DXMATRIX          m_mLocal;
	D3DLIGHT9           m_lit;
	ID3DXMesh* m_pMesh;
	d3d::BoundingSphere m_bound;
};


class Player {
private:
	CSphere ball[4]; // �� �÷��̾ ���� ��
	int playerNum;
	int score;

public:
	Player() {
		playerNum = 1;
		score = 0;
	}
	Player(int n) {
		playerNum = n;
		score = 0;
	}

	bool createBalls() {
		int i = 0;
		D3DCOLOR color;
		if (playerNum == 1) color = d3d::RED;
		else if (playerNum == 2) color = d3d::YELLOW;
		for (i = 0; i < 4; i++) {
			if (false == ball[i].create(Device, color)) return false;
		}
		setBalls();
		return true;
	}
	void setBalls() {
		int i = 0;
		if (playerNum == 1) {
			for (i = 0; i < 4; i++) {
				this->ball[i].setCenter(spherePos[i][0], (float)M_RADIUS, spherePos[i][1]);
				this->ball[i].setPower(0, 0);
				this->ball[i].setIsPlaying(false);
			}
		}
		else if (playerNum == 2) {
			for (i = 0; i < 4; i++) {
				this->ball[i].setCenter(spherePos2[i][0], (float)M_RADIUS, spherePos2[i][1]);
				this->ball[i].setPower(0, 0);
				this->ball[i].setIsPlaying(false);
			}
		}
	}
	bool isAllStop() {
		int i = 0;
		for (i = 0; i < 4; i++) {
			if (!ball[i].isStop()) return false;
		}
		return true;
	}
	CSphere& getBall(int i) {
		return this->ball[i];
	}

	void setScore(int i) {
		score = i;
	}
	int getScore() {
		return score;
	}
	// distroy �߰�?
};


class Curling {
private:
	Player p[2];
	int max_turn; // �ִ� �� �� ( player�� ���� ������ ���ƾ� ��)
	int max_set; // �ִ� ��Ʈ ��
	int now_set; // ���� ���° ��Ʈ�ΰ�
	int now_turn; // ���� ���° ���ΰ�
	int total_score[2]; // ����
	int whose_turn; // ���� ��� player�� ���ΰ� 0 �̸� 1p 1�̸� 2p

public:
	Curling() {
		max_turn = 4;
		max_set = 2;
		now_set = 1;
		now_turn = 1;
		total_score[0] = 0;
		total_score[1] = 0;
		whose_turn = 0;
		p[0] = Player(1);
		p[1] = Player(2);
	}

	void scoreCheck() { // ��Ʈ�� ������ ��Ʈ������ �� ������ �ջ��Ѵ�.
		total_score[0] += p[0].getScore();
		total_score[1] += p[1].getScore();
	}

	std::string getWinnerStr() { //  ������ ���� ���� string�� ��ȯ�Ѵ�. �̴� display���� CText winner�� ���� ����ߴ�.
		std::string str;
		if (total_score[0] > total_score[1])
			str = "Player 1 Win!!";
		else if (total_score[0] < total_score[1])
			str = "Player 2 Win!!";
		else
			str = "Draw!!";

		return str;

	}
	void nextSet() { // ��� ���� ����Ǹ� ȣ��Ǿ� ���� ��Ʈ�� �����ϴ� �Լ�. 
		scoreCheck(); // ����üũ
		Sleep(2000); // ��Ʈ ��� 2�ʵ��� ����(������ ����� Ȯ���ϴ� �ð�)
		p[0].setBalls(); // �� ��ġ �ʱ�ȭ
		p[1].setBalls();
		now_turn = 1; // �ٽ� ù��° ������
		now_set++; // ���� ��Ʈ�� 1 ����
		whose_turn = now_set % 2 == 1 ? 0 : 1; // Ȧ�� ��Ʈ�̸� 1p����, �ƴϸ� 2p���� ����.
	}

	bool isAllStop() { // 1p�� 2p�� ��� ���� ���߾����� true
		if (p[0].isAllStop() && p[1].isAllStop()) return true;
		else return false;
	}
	void setScoreC() { // �ø�ó�� ������ �����ϴ� �Լ�. 
		float p1[4] = { 99,99,99,99 };
		float p2[4] = { 99,99,99,99 };
		int score_p1 = 0;
		int score_p2 = 0;
		CSphere cs1;
		CSphere cs2;


		for (int i = 0; i < getMaxTurn(); i++) {
			cs1 = getPlayer(0).getBall(i);
			cs2 = getPlayer(1).getBall(i);
			if (cs1.getIsPlaying()) {
				cs1.setDistance();
				p1[i] = cs1.getDistance();
			}
			if (cs2.getIsPlaying()) {
				cs2.setDistance();
				p2[i] = cs2.getDistance();
			}
		}

		std::sort(p1, p1 + 4);
		std::sort(p2, p2 + 4);

		if (p1[0] < p2[0]) {
			score_p1++;
			for (int i = 1; i < getMaxTurn(); i++) {
				if (p1[i] < p2[0]) {
					score_p1++;
				}
				else break;
			}
		}
		else if (p1[0] > p2[0]) {
			score_p2++;
			for (int i = 1; i < getMaxTurn(); i++) {
				if (p2[i] < p1[0]) {
					score_p2++;
				}
				else break;
			}
		}
		getPlayer(0).setScore(score_p1);
		getPlayer(1).setScore(score_p2);

	}
	void nextTurn() {
		if (now_set % 2 == 1) {
			if (whose_turn == 1) now_turn++; // 2p�� ���ʰ� �����ٸ� ���� ������ �̵�
			changePlayer(); // ���ʰ� ������ �÷��̾� ��ü
		}
		else {
			if (whose_turn == 0) now_turn++; // 2p�� ���ʰ� �����ٸ� ���� ������ �̵�
			changePlayer(); // ���ʰ� ������ �÷��̾� ��ü
		}
	}

	Player& getPlayer(int num) { return p[num]; }
	int getMaxTurn() { return max_turn; }
	int getMaxSet() { return max_set; }
	int getNowTurn() { return now_turn; }
	int getNowSet() { return now_set; }
	int getWhoseTurn() { return whose_turn; }
	int getScore(int i) { return total_score[i]; }
	void setNowTurn(int i) { this->now_turn = i; }
	void changePlayer() { // ���� �÷��̾ �ٲ۴�
		whose_turn = whose_turn == 0 ? 1 : 0;
	}


};


// -----------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------
CWall	g_legoPlane;
CWall	g_legowall[4];
CSphere	g_sphere[4];
CSphere	g_target_blueball;
CLight	g_light;
CText   g_score; // ȭ�� ��� ��� score ǥ��
CText	g_set; // score �ٷ� �Ʒ��� ��Ʈ ǥ��
CText   g_player1; // ȭ�� ��� ������ P1�� ���� ��Ʈ ����ǥ��
CText   g_player2; // ȭ�� ��� ������ P2�� ���� ��Ʈ ����ǥ��
CText	g_totalP1; // ȭ�� ��� ������ P1�� ����ǥ��
CText	g_totalP2; // ȭ�� ��� ������ P2�� ����ǥ��
CText	g_winner; // ��� ��Ʈ ������ ���� ǥ��
CText	g_timer; // �ð����� ǥ��
float	sec; //�� ���� ���
float	elapsedTime; // ����ð� ǥ��
Curling game; // Curling ��ü �������� ����

double g_camera_pos[3] = { 0.0, 5.0, -8.0 };

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------


void destroyAllLegoBlock(void)
{
}

// initialization
bool Setup()
{
	int i;

	//setup text
	if (g_score.create(Device, Width, Height, "SCORE") == false) return false;
	if (g_player1.create(Device, Width, Height, "0") == false) return false;
	if (g_player2.create(Device, Width - 30, Height, "0") == false) return false;
	if (g_set.create(Device, Width, Height, "SET") == false) return false;
	if (g_totalP1.create(Device, Width, Height, "Player1 : ") == false) return false;
	if (g_totalP2.create(Device, Width - 30, Height, "Player2 : ") == false) return false;
	if (g_winner.create(Device, Width, Height, " ") == false) return false;
	if (g_timer.create(Device, Width, Height-80, std::to_string(TIME_LIMIT)) == false) return false;

	g_player1.setColor(d3d::RED);
	g_player2.setColor(d3d::YELLOW);
	g_totalP1.setColor(d3d::RED);
	g_totalP2.setColor(d3d::YELLOW);
	g_timer.setColor(d3d::BLACK);
	g_player1.setPosition(0, 50);
	g_player2.setPosition(0, 50);
	g_set.setPosition(0, 50);
	g_winner.setPosition(0, 100);
	g_score.setAnchor(DT_TOP | DT_CENTER);
	g_player1.setAnchor(DT_TOP | DT_LEFT);
	g_player2.setAnchor(DT_TOP | DT_RIGHT);
	g_set.setAnchor(DT_TOP | DT_CENTER);
	g_totalP1.setAnchor(DT_TOP | DT_LEFT);
	g_totalP2.setAnchor(DT_TOP | DT_RIGHT);
	g_winner.setAnchor(DT_TOP | DT_CENTER);
	g_timer.setAnchor(DT_BOTTOM | DT_CENTER);
	
	D3DXMatrixIdentity(&g_mWorld);
	D3DXMatrixIdentity(&g_mView);
	D3DXMatrixIdentity(&g_mProj);

	// create plane and set the position
	if (false == g_legoPlane.create(Device, -1, -1, 6, 0.03f, 9, d3d::GREEN)) return false;
	g_legoPlane.setPosition(0.0f, -0.0006f / 5, 0.0f);


	// create walls and set the position. note that there are four walls
	if (false == g_legowall[0].create(Device, -1, -1, 6, 0.3f, 0.12f, d3d::DARKRED)) return false;
	g_legowall[0].setPosition(0.0f, 0.12f, 4.56f);
	if (false == g_legowall[1].create(Device, -1, -1, 6, 0.3f, 0.12f, d3d::DARKRED)) return false;
	g_legowall[1].setPosition(0.0f, 0.12f, -4.56f);
	if (false == g_legowall[2].create(Device, -1, -1, 0.12f, 0.3f, 9.24f, d3d::DARKRED)) return false;
	g_legowall[2].setPosition(3.06f, 0.12f, 0.0f);
	if (false == g_legowall[3].create(Device, -1, -1, 0.12f, 0.3f, 9.24f, d3d::DARKRED)) return false;
	g_legowall[3].setPosition(-3.06f, 0.12f, 0.0f);


	if (false == game.getPlayer(0).createBalls()) return false; // Curling�� ��� Player�� ������ �ִ� ���� ���� (4��)
	if (false == game.getPlayer(1).createBalls()) return false;


	// create blue ball for set direction
	if (false == g_target_blueball.create(Device, d3d::BLUE)) return false;
	g_target_blueball.setCenter(.0f, (float)M_RADIUS, .0f);

	// light setting 
	D3DLIGHT9 lit;
	::ZeroMemory(&lit, sizeof(lit));
	lit.Type = D3DLIGHT_POINT;
	lit.Diffuse = d3d::WHITE;
	lit.Specular = d3d::WHITE * 0.9f;
	lit.Ambient = d3d::WHITE * 0.9f;
	lit.Position = D3DXVECTOR3(0.0f, 3.0f, 0.0f);
	lit.Range = 100.0f;
	lit.Attenuation0 = 0.0f;
	lit.Attenuation1 = 0.9f;
	lit.Attenuation2 = 0.0f;
	if (false == g_light.create(Device, lit))
		return false;

	// Position and aim the camera.
	D3DXVECTOR3 pos(0.0f, 14.0f, -1.0f); // ���� ī�޶� ��ġ 
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f); // ī�޶� ���ߴ� ��
	D3DXVECTOR3 up(0.0f, 2.0f, 0.0f);
	D3DXMatrixLookAtLH(&g_mView, &pos, &target, &up);
	Device->SetTransform(D3DTS_VIEW, &g_mView);

	// Set the projection matrix.
	D3DXMatrixPerspectiveFovLH(&g_mProj, D3DX_PI / 4,
		(float)Width / (float)Height, 1.0f, 100.0f);
	Device->SetTransform(D3DTS_PROJECTION, &g_mProj);

	// Set render states.
	Device->SetRenderState(D3DRS_LIGHTING, TRUE);
	Device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
	Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);

	g_light.setLight(Device, g_mWorld);
	return true;
}

void Cleanup(void)
{
	g_legoPlane.destroy();
	for (int i = 0; i < 4; i++) {
		g_legowall[i].destroy();
	}
	destroyAllLegoBlock();
	g_light.destroy();
}


// timeDelta represents the time between the current image frame and the last image frame.
// the distance of moving balls should be "velocity * timeDelta"
bool Display(float timeDelta)
{
	int i = 0;
	int j = 0;
	int turn = game.getNowTurn(); // ���� ���° ���ΰ�

	// ����� �Է��� ������(== ���� ��� ����) ���¿����� Ÿ�̸� �۵�
	if (game.isAllStop()) {
		sec += timeDelta;
		elapsedTime += timeDelta;
	}
	

	if (Device)
	{
		Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00afafaf, 1.0f, 0);
		Device->BeginScene();

		// �� ���� ����. ���� ��� ���߾����� ���� ���� ����.
		if (game.isAllStop()) {
			if (turn > game.getMaxTurn()) { // ���� ���� max�� �ʰ��ߴٸ� ���� ��Ʈ�� �����Ѵ�.
				turn = 1;
				game.nextSet();
				if (game.getNowSet() > game.getMaxSet()) { // ���� ��Ʈ�� max�� �ʰ��ߴٸ� �º� ����� ����Ѵ�.
					g_winner.setStr(game.getWinnerStr());
				}
			}
			game.getPlayer(game.getWhoseTurn()).getBall(turn - 1).setCenter(0, (float)M_RADIUS, -4); // �� ������ ���� ��߼��� ��ġ��Ų��.
		}


		// update the position of each ball. during update, check whether each ball hit by walls.
		for (i = 0; i < 4; i++) {
			game.getPlayer(0).getBall(i).ballUpdate(timeDelta);
			game.getPlayer(1).getBall(i).ballUpdate(timeDelta);
			for (j = 0; j < 4; j++) {
				g_legowall[i].hitBy(game.getPlayer(0).getBall(j));
				g_legowall[i].hitBy(game.getPlayer(1).getBall(j));
			}
		}

		// check whether any two balls hit together and update the direction of balls
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++) {
				if (i >= j) { continue; }
				game.getPlayer(0).getBall(i).hitBy(game.getPlayer(0).getBall(j)); // P1 �� ������ üũ
				game.getPlayer(1).getBall(i).hitBy(game.getPlayer(1).getBall(j)); // P2 �� ������ üũ
			}
			for (j = 0; j < 4; j++) {
				game.getPlayer(0).getBall(i).hitBy(game.getPlayer(1).getBall(j)); // P1�� P2�� ������ üũ
			}
		}

		// �ǽð� ���� ����
		if(game.isAllStop()) game.setScoreC();

		// draw text
		g_player1.setStr(" " + std::to_string(game.getPlayer(0).getScore()));
		g_player2.setStr(" " + std::to_string(game.getPlayer(1).getScore()));
		g_totalP1.setStr("Player1: " + std::to_string(game.getScore(0)));
		g_totalP2.setStr("Player2: " + std::to_string(game.getScore(1)));
		g_set.setStr("SET " + std::to_string(game.getNowSet()) + "/" + std::to_string(game.getMaxSet()));
		g_score.draw();
		g_player1.draw();
		g_player2.draw();
		g_totalP1.draw();
		g_totalP2.draw();
		g_set.draw();
		g_winner.draw();
		g_timer.draw();

		// TIME_LIMIT �ʺ��� 1�ʸ��� �پ��� �ð� ǥ��
		if (sec > 1.0f) {
			sec = 0;
			g_timer.setStr(std::to_string((TIME_LIMIT - (int)elapsedTime)));
		}
		// TIME_LIMIT �ʸ�ŭ �ð� ��� ��, ���� ������ �Ѿ
		if (elapsedTime > (float)TIME_LIMIT) {
			elapsedTime = 0;
			game.nextTurn();
		}
		
		// draw plane, walls, and spheres
		g_legoPlane.draw(Device, g_mWorld);
		for (i = 0; i < 4; i++) {
			g_legowall[i].draw(Device, g_mWorld);
			game.getPlayer(0).getBall(i).draw(Device, g_mWorld);
			game.getPlayer(1).getBall(i).draw(Device, g_mWorld);
		}
		g_target_blueball.draw(Device, g_mWorld);
		g_light.draw(Device);

		Device->EndScene();
		Device->Present(0, 0, 0, 0);
		Device->SetTexture(0, NULL);
	}
	return true;
}

LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static bool wire = false;
	static bool isReset = true;
	static int old_x = 0;
	static int old_y = 0;
	static enum { WORLD_MOVE, LIGHT_MOVE, BLOCK_MOVE } move = WORLD_MOVE;

	switch (msg) {
	case WM_DESTROY:
	{
		::PostQuitMessage(0);
		break;
	}
	case WM_KEYDOWN:
	{
		switch (wParam) {
		case VK_ESCAPE:
			::DestroyWindow(hwnd);
			break;
		case VK_RETURN:
			if (NULL != Device) {
				wire = !wire;
				Device->SetRenderState(D3DRS_FILLMODE,
					(wire ? D3DFILL_WIREFRAME : D3DFILL_SOLID));
			}
			break;
		case VK_SPACE:

			int i;
			int turn = game.getNowTurn();
			int player = game.getWhoseTurn();
			// ���� ���� ������ ��ٸ�
			if (turn > 0) {
				for (i = 0; i < 4; i++) {
					if (!game.isAllStop()) goto HERE;
				}
			}

			D3DXVECTOR3 targetpos = g_target_blueball.getCenter();
			D3DXVECTOR3	whitepos = game.getPlayer(player).getBall(turn - 1).getCenter();

			double theta = acos(sqrt(pow(targetpos.x - whitepos.x, 2)) / sqrt(pow(targetpos.x - whitepos.x, 2) +
				pow(targetpos.z - whitepos.z, 2)));		// �⺻ 1 ��и�
			if (targetpos.z - whitepos.z <= 0 && targetpos.x - whitepos.x >= 0) { theta = -theta; }	//4 ��и�
			if (targetpos.z - whitepos.z >= 0 && targetpos.x - whitepos.x <= 0) { theta = PI - theta; } //2 ��и�
			if (targetpos.z - whitepos.z <= 0 && targetpos.x - whitepos.x <= 0) { theta = PI + theta; } // 3 ��и�
			double distance = sqrt(pow(targetpos.x - whitepos.x, 2) + pow(targetpos.z - whitepos.z, 2));
			game.getPlayer(player).getBall(turn - 1).setPower(distance * cos(theta), distance * sin(theta));
			game.getPlayer(player).getBall(turn - 1).setIsPlaying(true); // player�� ���� �������� ���ӿ� �������� ���� �ȴ�.
			sec = 0;
			elapsedTime = 0;
			game.nextTurn();

		HERE:
			break;

		}
		break;
	}

	case WM_MOUSEMOVE:
	{
		int new_x = LOWORD(lParam);
		int new_y = HIWORD(lParam);
		float dx;
		float dy;
		float tx;
		float tz;

		isReset = true;

		if (LOWORD(wParam) & MK_RBUTTON) {
			dx = (old_x - new_x);// * 0.01f;
			dy = (old_y - new_y);// * 0.01f;

			D3DXVECTOR3 coord3d = g_target_blueball.getCenter();
			tx = coord3d.x + dx * (-0.007f);
			tz = coord3d.z + dy * 0.007f;
			if (tx >= 3 - M_RADIUS) tx = 3 - M_RADIUS;
			else if (tx <= -3 + M_RADIUS) tx = -3 + M_RADIUS;
			if (tz >= 4.5 - M_RADIUS) tz = 4.5 - M_RADIUS;
			else if (tz <= -4.5 + M_RADIUS) tz = -4.5 + M_RADIUS;
			g_target_blueball.setCenter(tx, coord3d.y, tz);
		}
		old_x = new_x;
		old_y = new_y;

		move = WORLD_MOVE;

		break;
	}
	}

	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hinstance,
	HINSTANCE prevInstance,
	PSTR cmdLine,
	int showCmd)
{
	srand(static_cast<unsigned int>(time(NULL)));

	if (!d3d::InitD3D(hinstance,
		Width, Height, true, D3DDEVTYPE_HAL, &Device))
	{
		::MessageBox(0, "InitD3D() - FAILED", 0, 0);
		return 0;
	}

	if (!Setup())
	{
		::MessageBox(0, "Setup() - FAILED", 0, 0);
		return 0;
	}

	d3d::EnterMsgLoop(Display);

	Cleanup();

	Device->Release();

	return 0;
}