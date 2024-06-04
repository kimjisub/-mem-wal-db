#include "hash_table.h"

/**
 * Hash table을 구현하기 위해서 Quadratic Probing을 사용합니다.
 * 해시 스토리지를 만들 때, key를 해싱하고, 그 값을 인덱스로 사용하되,
 * 만약 이미 있다면 2^0칸 다음, 만약 또 이미 있다면 2^1, 그다음은 2^2, 2^3, ...,
 * 2^n 이렇게 저장하고, 가져올 때에도 이 규칙에 따라 가져오는 방식입니다. 자동
 * 리사이징은 구현하지 않았으며, 모든 테이블이 꽉 찬 경우 무한 루프에 빠지지
 * 않도록 구현하였습니다.
 */

/**
 * Hash table을 초기화합니다.
 * is_occupied를 0으로 초기화합니다.
 */
void hashtable_init(HashTable *table) {
    for (int i = 0; i < TABLE_SIZE; ++i) {
        table->entries[i].is_occupied = 0;
    }
}

/**
 * key를 해싱하여 테이블의 크기로 나눈 나머지를 반환합니다.
 */
unsigned int hash(const char *key) {
    unsigned int hash = 0;
    while (*key) { // null 문자가 나올 때까지
        hash = (hash << 5) + *key++;
    }

    return hash % TABLE_SIZE;
}

/**
 * 해당 key가 어느 인덱스에 있는지 찾아서 반환합니다.
 * 찾지 못했을 경우 -1을 반환합니다.
 */
int hashtable_find_index(HashTable *table, const char *key) {
    unsigned int index = hash(key);

    for (int i = 0; i < TABLE_SIZE; i++) {
        index = (index + (i << 2)) % TABLE_SIZE;

        // 찾았다면, 해당 인덱스를 반환
        if (table->entries[index].is_occupied &&
            strcmp(table->entries[index].key, key) == 0) {
            return index;
        }

        // 빈 공간이 발견됐다면, 찾지 못했으므로 -1을 반환
        if (!table->entries[index].is_occupied) {
            return -1;
        }
    }

    // 테이블 전체를 돌았는데도 찾지 못했다면, -1을 반환
    return -1;
}

/**
 * 해당 key가 어느 인덱스에 있는지 찾아서 반환합니다.
 * 찾지 못했을 경우, 들어갈 수 있는 다음 인덱스를 반환합니다.
 */
int hashtable_get_next_index(HashTable *table, const char *key) {
    unsigned int index = hash(key);

    for (int i = 0; i < TABLE_SIZE; i++) {
        index = (index + (i << 2)) % TABLE_SIZE;

        // 찾았다면, 해당 인덱스를 반환
        if (table->entries[index].is_occupied &&
            strcmp(table->entries[index].key, key) == 0) {
            return index;
        }

        // 빈 공간이 있다면, 해당 인덱스를 반환
        if (!table->entries[index].is_occupied) {
            return index;
        }
    }

    // 위 두 경우가 아니라면, -1을 반환
    return -1;
}

int hashtable_set(HashTable *table, const char *key, const char *value) {
    unsigned int index = hashtable_get_next_index(table, key);

    // 더이상 넣을 공간을 찾지 못했다면, 오류 발생
    if (index == -1) {
        return -1;
    }

    strcpy(table->entries[index].key, key);
    strcpy(table->entries[index].value, value);
    table->entries[index].is_occupied = 1;

    return 0;
}

int hashtable_get(HashTable *table, const char *key, char *value,
                  size_t buffer_size) {
    unsigned int index = hashtable_find_index(table, key);

    // 찾지 못했다면, 실패
    if (index == -1) {
        return -1;
    }

    // 찾았다면, 값을 복사
    strncpy(value, table->entries[index].value, buffer_size - 1);
    value[buffer_size - 1] = '\0'; // 널 종료 문자 추가

    return 0;
}

int hashtable_del(HashTable *table, const char *key) {
    unsigned int index = hashtable_find_index(table, key);

    // 찾지 못했다면, 실패
    if (index == -1) {
        return -1;
    }

    // 찾았다면, 값을 삭제
    table->entries[index].is_occupied = 0;

    return 0;
}

/**
 * 해당 key가 존재하는지 확인합니다.
 * 존재한다면, 해당 인덱스를 반환합니다.
 * 존재하지 않는다면, -1을 반환합니다.
 */
int hashtable_exist_key(HashTable *table, const char *key) {
    int index = hashtable_find_index(table, key);

    return index;
}