#pragma once
#define MAX_VALUE_PER_POOLS 1
typedef char uint16_t;
#include <iostream>
using namespace std;

struct Node {
private:
	Node* m_pNext;
	bool m_is_pool_full = false;
	unsigned int m_contained_points_count = 0;

public:
	uint16_t m_points[MAX_VALUE_PER_POOLS];
	void Add(uint16_t _target) {
		m_points[m_contained_points_count++] = _target;
		if (m_contained_points_count >= MAX_VALUE_PER_POOLS) {
			m_is_pool_full = true;
			//cout << "풀이 곽차서 다음으로 이관합니다." << endl;
		}
	}
	bool IsContain(uint16_t _target) {
		for (int index = 0; index < MAX_VALUE_PER_POOLS; index++) {
			if (m_points[index] == _target) return true;
		}
		return false;
	}
	bool IsContain(uint16_t _target, int* return_index) {
		for (int index = 0; index < MAX_VALUE_PER_POOLS; index++) {
			if (m_points[index] == _target) {
				*return_index = index;
				return true;
			}
		}
		*return_index = -1;
		return false;
	}

	bool IsFull() {
		return m_is_pool_full;
	}
	Node* GetNext() {
		return m_pNext;
	}
	void SetNext(Node** _target) {
		m_pNext = *_target;
	}
};
class UintPool {
	Node* m_pStart = nullptr, * m_pEnd = nullptr;
	uint16_t* m_data = nullptr;
	unsigned int m_count = 0;
public:
	void Add(uint16_t _target) {
		if (m_pStart == nullptr) {

			m_pStart = new Node();
			if (m_pStart != nullptr) {
				m_pStart->Add(_target);
				m_pEnd = m_pStart;
				m_count++;
			}
			else cout << "힙메모리 과다사용" << endl;
		}
		else {
			if (m_pEnd->IsFull()) {
				Node* tmp = new Node();
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

	uint16_t* operator[](const unsigned int index) {
		return Refer(index);
	}
	uint16_t* Refer(const unsigned int index) {
		Node* _iter = m_pStart;
		for (int _iter_count = 0; _iter != nullptr && _iter_count < index / MAX_VALUE_PER_POOLS; _iter = _iter->GetNext(), _iter_count++);
		if (_iter != nullptr) return &_iter->m_points[index % MAX_VALUE_PER_POOLS];
		else return nullptr;
	}
	uint16_t* GetData()
	{
		if (m_data != nullptr)
			return m_data;
		m_data = new uint16_t[static_cast<size_t>(m_count)];
		for (int i = 0; i < m_count; i++)
		{
			const uint16_t* ref = this->Refer(i);
			if (ref != nullptr)
				m_data[i] = *ref;
			else
				m_data[i] = 0;
		}
		return m_data;
	}
	bool IsContain(uint16_t _target) {
		Node* _iter = m_pStart;
		while (_iter != nullptr) {
			if (_iter->IsContain(_target)) return true;
			else _iter = _iter->GetNext();
		}
		return false;
	}
	bool IsContain(uint16_t _target, int* return_index) {
		Node* _iter = m_pStart;
		int _iter_count = 0;
		int tmp_return_index = 0;
		while (_iter != nullptr) {
			if (_iter->IsContain(_target, &tmp_return_index)) {
				*return_index = (_iter_count * MAX_VALUE_PER_POOLS) + tmp_return_index;
				return true;
			}
			else {
				_iter = _iter->GetNext();
				_iter_count++;
			}
		}
		*return_index = -1;
		return false;
	}
	unsigned int GetCount() {
		return m_count;
	}
	~UintPool() {

	}
	void Release()
	{
		if (m_data != nullptr)
			delete[] m_data;
		Node* _iter = m_pStart, * _iter_copy = nullptr;
		while (_iter != nullptr) {
			_iter_copy = _iter;
			_iter = _iter->GetNext();
			delete _iter_copy;
			//cout << "풀 하나를 할당해제합니다." << endl;
		}
	}
};