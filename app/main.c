#include "expression.h"
#include "acquisition.h"
#include "ds/cqueue.h"
#include "ds/seqstack.h"
#include "ds/liststack.h"
#include "ds/listqueue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

typedef struct { int next; } DemoSensor;
static int simulated_sensor(void *context, int *value) {
    DemoSensor *sensor = context;
    *value = sensor->next++;
    return 0;
}
static int acquisition_demo(void) {
    DemoSensor sensor = {100};
    Acquisition acquisition;
    if (acq_init(&acquisition, 4, Q_SPARE_SLOT, ACQ_DROP_OLDEST, simulated_sensor, &sensor)) return 1;
    if (acq_start(&acquisition)) { acq_destroy(&acquisition); return 1; }
    puts("数据采集演示：模拟传感器产生100至107，有效缓冲4格，满时覆盖最旧样本。");
    for (int i = 0; i < 8; ++i) if (acq_poll(&acquisition) != ACQ_OK) { acq_destroy(&acquisition); return 1; }
    acq_stop(&acquisition);
    int value;
    printf("停止采样后FIFO读取：");
    while (acq_read(&acquisition, &value) == ACQ_OK) printf(" %d", value);
    puts("");
    AcqStats stats;
    if (acq_get_stats(&acquisition, &stats)) { acq_destroy(&acquisition); return 1; }
    printf("统计：采样=%llu，读取=%llu，丢弃=%llu，源错误=%llu，待读=%zu\n",
        (unsigned long long)stats.sampled, (unsigned long long)stats.consumed,
        (unsigned long long)stats.dropped, (unsigned long long)stats.source_errors,
        acq_pending(&acquisition));
    acq_destroy(&acquisition);
    return 0;
}
static int demo(void) {
    if (acquisition_demo()) return 1;
    CirQueue q;
    SeqStack s;
    ListStack linked;
    ListQueue lq;
    int v;
    if (q_init(&q, 5)) return 1;
    puts("循环队列：物理5格，有效4格；先入1,2,3,4，出1,2，再入5,6。");
    for (int i = 1; i <= 4; ++i) if (q_enqueue(&q, i)) { q_destroy(&q); return 1; }
    for (int i = 0; i < 2; ++i) if (q_dequeue(&q, &v)) { q_destroy(&q); return 1; }
    if (q_enqueue(&q, 5) || q_enqueue(&q, 6)) { q_destroy(&q); return 1; }
    printf("绕环后 front=%d rear=%d，FIFO出队：", q.front, q.rear);
    while (q_dequeue(&q, &v) == 0) printf(" %d", v);
    puts(""); q_destroy(&q);
    if (s_init(&s, 3)) return 1;
    for (int i = 1; i <= 3; ++i) if (s_push(&s, i)) { s_destroy(&s); return 1; }
    printf("顺序栈压入1,2,3，LIFO弹出：");
    while (s_pop(&s, &v) == 0) printf(" %d", v);
    puts(""); s_destroy(&s);
    ls_init(&linked);
    if (ls_push(&linked, 100) || ls_push(&linked, 200)) { ls_destroy(&linked); return 1; }
    printf("链栈撤销操作编号：");
    while (ls_pop(&linked, &v) == 0) printf(" %d", v);
    puts(""); ls_destroy(&linked);
    lq_init(&lq);
    if (lq_enqueue(&lq, 10) || lq_enqueue(&lq, 20)) { lq_destroy(&lq); return 1; }
    printf("链式队列出队：");
    while (lq_dequeue(&lq, &v) == 0) printf(" %d", v);
    puts(""); lq_destroy(&lq);
    const char *brackets = "{[()]}";
    printf("括号匹配 %s：%s\n", brackets, check_brackets(brackets) == 1 ? "合法" : "不合法");
    double result;
    const char *expression = "-(2+3)*4+10/2";
    if (expression_eval(expression, &result, NULL) != EXPR_OK) return 1;
    printf("表达式 %s = %.12g\n", expression, result);
    return 0;
}
static int evaluate(const char *text) {
    double result;
    size_t pos;
    ExprStatus status = expression_eval(text, &result, &pos);
    if (status != EXPR_OK) {
        fprintf(stderr, "求值失败：%s（检测位置：第%zu字节，从0计）\n", expression_status(status), pos);
        return 1;
    }
    printf("%s = %.12g\n", text, result);
    return 0;
}
int main(int argc, char **argv) {
#ifdef _WIN32
    SetErrorMode(GetErrorMode() | SEM_NOGPFAULTERRORBOX);
    (void)SetConsoleOutputCP(CP_UTF8);
    (void)SetConsoleCP(CP_UTF8);
#endif
    if (argc == 3 && strcmp(argv[1], "--eval") == 0) return evaluate(argv[2]);
    if (argc == 2 && strcmp(argv[1], "--acquisition") == 0) return acquisition_demo();
    if (argc == 3 && strcmp(argv[1], "--brackets") == 0) {
        int matched = check_brackets(argv[2]);
        printf("括号检查：%s\n", matched == 1 ? "匹配" : matched == 0 ? "不匹配" : "输入过长或资源不足");
        return matched == 1 ? 0 : 1;
    }
    if (argc == 2 && strcmp(argv[1], "--demo") == 0) return demo();
    if (argc != 1) {
        fputs("用法：lab.exe [--acquisition | --demo | --eval \"表达式\" | --brackets \"括号串\"]\n", stderr);
        return 2;
    }
    if (demo()) return 1;
    puts("\n输入四则表达式并回车；输入 q 退出。支持小数、括号和一元正负号。");
    char line[EXPR_MAX_LENGTH + 3U];
    while (1) {
        fputs("> ", stdout); fflush(stdout);
        if (!fgets(line, (int)sizeof(line), stdin)) break;
        size_t n = strlen(line);
        if (n && line[n - 1] != '\n' && !feof(stdin)) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
            fputs("输入过长，本行已丢弃。\n", stderr);
            continue;
        }
        line[strcspn(line, "\r\n")] = '\0';
        if (strcmp(line, "q") == 0 || strcmp(line, "quit") == 0) break;
        if (*line) (void)evaluate(line);
    }
    return 0;
}
