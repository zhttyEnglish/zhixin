#ifndef _TEST_PINCTRL_H
#define _TEST_PINCTRL_H

struct pinctrl_chip
{
    int (*list)(void);
    int (*get)(int pin_index);
    int (*set)(int pin_index, unsigned int pin_config);

    void (*usage)(const char *name);
};

int sca200v100_pinctrl_chip(struct pinctrl_chip *pchip, const char *comp);
int sca200v200_pinctrl_chip(struct pinctrl_chip *pchip, const char *comp);

#endif /* _TEST_PINCTRL_H */
