#include "config_read.h"

// 简单的INI读取函数（仅演示，未做健壮性处理）
void trim(char *str) {
    char *end;
    while(*str == ' ' || *str == '\t') str++;
    end = str + strlen(str) - 1;
    while(end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
    *(end+1) = 0;
}

void parse_config(const char *filename, Config *cfg) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        exit(1);
    }
    char line[512];
    int cam_idx = -1;
    int max_cam_idx = -1;
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (line[0] == ';' || line[0] == 0) continue;
        if (line[0] == '[') {
            if (strncmp(line, "[Cam", 4) == 0) {
                cam_idx = atoi(&line[4]) - 1;
                if (cam_idx > max_cam_idx) max_cam_idx = cam_idx;
            } else {
                cam_idx = -1;
            }
            continue;
        }
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        char *key = line;
        char *val = eq + 1;
        trim(key); trim(val);
        if (cam_idx >= 0 && cam_idx < MAX_CAM) {
            Cam *cam = &cfg->cams[cam_idx];
            if (strcmp(key, "cameraid") == 0) strncpy(cam->cameraid, val, MAX_STR_LEN);
            else if (strcmp(key, "useperson") == 0) cam->useperson = strcmp(val, "True") == 0;
            else if (strcmp(key, "useanimal") == 0) cam->useanimal = strcmp(val, "True") == 0;
            else if (strcmp(key, "usedown") == 0) cam->usedown = strcmp(val, "True") == 0;
            else if (strcmp(key, "usefire") == 0) cam->usefire = strcmp(val, "True") == 0;
            else if (strcmp(key, "usesmog") == 0) cam->usesmog = strcmp(val, "True") == 0;
            else if (strcmp(key, "usewater") == 0) cam->usewater = strcmp(val, "True") == 0;
            else if (strcmp(key, "rect") == 0) {
                // rect=left,top,right,bottom,score,prob
                int left=0,top=0,right=0,bottom=0;
                float score=0,prob=0;
                sscanf(val, "%d,%d,%d,%d,%f,%f", &left, &top, &right, &bottom, &score, &prob);
                cam->objs.rect.left = left;
                cam->objs.rect.top = top;
                cam->objs.rect.right = right;
                cam->objs.rect.bottom = bottom;
                cam->objs.score = score;
                cam->objs.prob = prob;
            }
            else if (strcmp(key, "osd") == 0) cam->osd = strcmp(val, "True") == 0;
            else if (strcmp(key, "frameRate") == 0) {
                if (strcmp(val, "h") == 0)
                    cam->frameRate = FRAME_RATE_HIGH;
                else if (strcmp(val, "m") == 0)
                    cam->frameRate = FRAME_RATE_MEDIUM;
                else if (strcmp(val, "l") == 0)
                    cam->frameRate = FRAME_RATE_LOW;
                else
                    cam->frameRate = FRAME_RATE_UNKNOWN;
            }
            else if (strcmp(key, "camcode") == 0) strncpy(cam->camcode, val, MAX_STR_LEN);
        } else {
            if (strcmp(key, "license") == 0) strncpy(cfg->license, val, MAX_STR_LEN);
            else if (strcmp(key, "deviceid") == 0) strncpy(cfg->deviceid, val, MAX_STR_LEN);
            else if (strcmp(key, "camcount") == 0) strncpy(cfg->camcount, val, MAX_STR_LEN);
            else if (strcmp(key, "sn") == 0) strncpy(cfg->sn, val, MAX_STR_LEN);
            else if (strcmp(key, "pd") == 0) cfg->pd = strcmp(val, "True") == 0;
            else if (strcmp(key, "linesize") == 0) cfg->linesize = atoi(val);
            else if (strcmp(key, "fire") == 0) cfg->algo.fire = atof(val);
            else if (strcmp(key, "down") == 0) cfg->algo.down = atof(val);
            else if (strcmp(key, "animal") == 0) cfg->algo.animal = atof(val);
            else if (strcmp(key, "person") == 0) cfg->algo.person = atof(val);
            else if (strcmp(key, "smog") == 0) cfg->algo.smog = atof(val);
            else if (strcmp(key, "water") == 0) cfg->algo.water = atof(val);
            else if (strcmp(key, "broker") == 0) strncpy(cfg->mqtt.broker, val, MAX_STR_LEN);
            else if (strcmp(key, "port") == 0) cfg->mqtt.port = atoi(val);
        }
    }
    fclose(fp);
    cfg->cam_num = max_cam_idx + 1;
}

// int main() {
//     Config cfg = {0};
//     parse_config("config.ini", &cfg);
//     printf("license: %s\n", cfg.license);
//     printf("deviceid: %s\n", cfg.deviceid);
//     printf("camcount: %s\n", cfg.camcount);
//     printf("sn: %s\n", cfg.sn);
//     printf("pd: %d\n", cfg.pd);
//     printf("linesize: %d\n", cfg.linesize);
//     printf("fire: %.2f, down: %.2f, animal: %.2f, person: %.2f, smog: %.2f, water: %.2f\n",
//         cfg.algo.fire, cfg.algo.down, cfg.algo.animal, cfg.algo.person, cfg.algo.smog, cfg.algo.water);
//     printf("mqtt broker: %s, port: %d\n", cfg.mqtt.broker, cfg.mqtt.port);
//     for (int i = 0; i < cfg.cam_num; ++i) {
//         Cam *cam = &cfg.cams[i];
//         printf("\n[Cam%d]\n", i+1);
//         printf("cameraid: %s\n", cam->cameraid);
//         printf("useperson: %d, useanimal: %d, usedown: %d, usefire: %d, usesmog: %d, usewater: %d\n",
//             cam->useperson, cam->useanimal, cam->usedown, cam->usefire, cam->usesmog, cam->usewater);
//         printf("rect: %d,%d,%d,%d, score: %.2f, prob: %.2f\n",
//             cam->objs.rect.left, cam->objs.rect.top, cam->objs.rect.right, cam->objs.rect.bottom,
//             cam->objs.score, cam->objs.prob);
//         printf("osd: %d\n", cam->osd);
//         printf("frameRate: ");
//         switch (cam->frameRate) {
//             case FRAME_RATE_HIGH: printf("h"); break;
//             case FRAME_RATE_MEDIUM: printf("m"); break;
//             case FRAME_RATE_LOW: printf("l"); break;
//             default: printf("unknown"); break;
//         }
//         printf("\n");
//         printf("camcode: %s\n", cam->camcode);
//     }
//     return 0;
// }
