#ifndef OWNERSHIP_H_INCLUDED
#define OWNERSHIP_H_INCLUDED


// 포인터를 함수나 객체에 넘겨줄 때 소유권의 이동을 표현하기 위한 매크로 모음입니다.
// const 키워드를 통해 접근을 제어합니다.
// const가 * 오른쪽에 위치하면 주소를 고정하고
// const가 * 왼쪽에 위치하면 값을 고정합니다.
// const T* a -> a는 const T를 가리키는 포인터입니다. 


// Mut은 Borrow-Mut을 의미하며
// 값을 수정 가능하지만 주소는 변경할 수 없다.
// ex: const int* 
#define Mut(x) x const

// Borrow는 값을 읽을 순 있지만 값을 수정하거나 주소를 변경할 수 없다.
// const int* const
#define Borrow(T) const T const

// Move는 소유권 자체가 이동했으므로 아무런 제약이 없다.
// int*
#define Move(T) T

// Out은 Borrow-Mut과 권한은 같지만 함수의 반환값 대신 주소 참조를 이용한다는 의미가 있다.
#define Out(T) T const

#endif // OWNERSHIP_H_INCLUDED
