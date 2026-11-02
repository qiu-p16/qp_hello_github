#ifndef SCORE_H
#define SCORE_H

/*============================================================
 * score.h  成绩管理模块
 * 负责人：孙汶
 * 查询结果以【单向链表】组织，排序采用【快速排序】，
 * 排名采用标准竞赛排名(同分同名次)。
 *==========================================================*/

#include "common.h"
#include "student.h"
#include "course.h"

typedef struct {
    char student_id[LEN_ID];
    char name[LEN_NAME];
    char class_name[LEN_CLASS];
    char course_id[LEN_ID];
    char course_name[COURSE_NAME_LEN];
    double score;
    char semester[24];
} ScoreRec;

/* 单向链表节点 */
typedef struct SNode {
    ScoreRec rec;
    struct SNode *next;
} SNode;

/* 结果集合(数组承载，便于快速排序) */
typedef struct {
    ScoreRec *items;
    int n;
} ScoreSet;

/* 增 / 改 / 删，成功返回0 */
int  score_add(const char *sid, const char *cid, double score, const char *sem);
int  score_update(const char *sid, const char *cid, double score);
int  score_delete(const char *sid, const char *cid);

/* 多条件组合查询(AND语义，空串表示该条件不限制)，结果需释放 */
ScoreSet *score_query(const char *sid, const char *cid,
                      const char *cls, const char *sem);
ScoreSet *score_query_by_student(const char *sid);
void      score_set_free(ScoreSet *s);

/* 快速排序：by_score=1按成绩、否则按学号；desc=1降序 */
void score_quicksort(ScoreRec *a, int lo, int hi, int by_score, int desc);
/* 排序后分页展示(每页 PAGE_SIZE 条) */
void score_show_paged(ScoreSet *s, int by_score, int desc);

/* 链表：由结果集构造、遍历、释放 */
SNode *score_list_build(const ScoreSet *s);
void   score_list_print(SNode *head);
void   score_list_free(SNode *head);

/* 统计分析：平均/最高/最低/及格率/优秀率 */
void score_stat(const char *cid, const char *cls, const char *sem);
/* 标准竞赛排名(按某课程) */
void score_rank(const char *cid, const char *sem);

#endif /* SCORE_H */
