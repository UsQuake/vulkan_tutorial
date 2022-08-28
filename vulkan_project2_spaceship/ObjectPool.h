#pragma once
#define MAX_VALUE_PER_POOLS 5
#include "RenderableObject.h"

//!_______________삭제시 순서가 바뀌는 것에 주의________________!
//! 
struct ObjectNode {
private:
	ObjectNode* m_pNext;
	bool m_is_pool_full = false;
	unsigned int m_contained_points_count = 0;

public:
	Renderable m_points[MAX_VALUE_PER_POOLS];
	void Add(Renderable _target) {
		m_points[m_contained_points_count++] = _target;
		if (m_contained_points_count >= MAX_VALUE_PER_POOLS) {
			m_is_pool_full = true;
		}
	}

	bool IsFull() {
		return m_is_pool_full;
	}
	ObjectNode* GetNext() {
		return m_pNext;
	}
	void SetNext(ObjectNode** _target) {
		m_pNext = *_target;
	}
};
class ObjectUnorderedPool {
	ObjectNode* m_pStart = nullptr, * m_pEnd = nullptr;
	Renderable* m_data = nullptr;
	unsigned int m_count = 0;
public:
	void Add(Renderable _target) {
		if (m_pStart == nullptr) {

			m_pStart = new ObjectNode();
			if (m_pStart != nullptr) {
				m_pStart->Add(_target);
				m_pEnd = m_pStart;
				m_count++;
			}
			else cout << "힙메모리 과다사용" << endl;
		}
		else {
			if (m_pEnd->IsFull()) {
				ObjectNode* tmp = new ObjectNode();
				if (tmp != nullptr) {
					m_pEnd->SetNext(&tmp);
					m_pEnd = tmp;
					m_pEnd->Add(_target);
					m_count++;
				}
				else cout << "힙메모리 과다사용" << endl;
			}
			else {
				m_pEnd->Add(_target);
				m_count++;
			}
		}

	}
	void Delete() //표준 삭제
	{
		
	}
	Renderable* operator[](const unsigned int index) {
		return Refer(index);
	}
	Renderable* Refer(const unsigned int index) {
		ObjectNode* _iter = m_pStart;
		for (int _iter_count = 0; _iter != nullptr && _iter_count < index / MAX_VALUE_PER_POOLS; _iter = _iter->GetNext(), _iter_count++);
		if (_iter != nullptr) return &_iter->m_points[index % MAX_VALUE_PER_POOLS];
		else return nullptr;
	}
	Renderable* GetData()
	{
		if (m_data != nullptr)
			return m_data;
		m_data = new Renderable[static_cast<size_t>(m_count)];
		for (int i = 0; i < m_count; i++)
		{
			const Renderable* ref = this->Refer(i);
			if (ref != nullptr)
				m_data[i] = *ref;
			else
				m_data[i] = Renderable();
		}
		return m_data;
	}
	unsigned int GetCount() {
		return m_count;
	}
	void Release()
	{
		if (m_data != nullptr)
			delete[] m_data;
		ObjectNode* _iter = m_pStart, * _iter_copy = nullptr;
		while (_iter != nullptr) {
			_iter_copy = _iter;
			_iter = _iter->GetNext();
			delete _iter_copy;
			//cout << "풀 하나를 할당해제합니다." << endl;
		}
	}
};